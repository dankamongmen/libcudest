#include <nv-misc.h>
#include <nv.h>
#include "cudest.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

// 195.36.15, 195.36.24, 256.22, 256.29
#define CUDARUNVER	"256.35"

#define DEVROOT "/dev/nvidia"
#define NVCTLDEV "/dev/nvidiactl"
#define NVCTLDEV_MODE (S_IFCHR | (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))
#define NVCTLDEV_MAJOR 195
#define NVCTLDEV_MINOR 255
#define NVNAMEMAX 0x84		// from the "get name" GPU method paramlen

#define MAX_CARDS 32 // FIXME pull from nv somehow? upstream constant

#define debug(s,...) fprintf(stderr,"%s:%d] "s,__func__,__LINE__,__VA_ARGS__)

typedef struct CUdevice_opaque {
	size_t regsize,fbsize;
	uintmax_t regaddr,fbaddr;
	unsigned arch;
	unsigned stepping;
	unsigned flags;
	unsigned irq;
	unsigned vendorid,deviceid,gpuid;
	unsigned pcidomain,busnumber,slot;
	int attrs[CU_DEVICE_ATTRIBUTE_ECC_ENABLED + 1];
	char name[NVNAMEMAX];
} CUdevice_opaque;

static CUdevice_opaque devs[MAX_CARDS];

// http://nouveau.freedesktop.org/wiki/HwIntroduction
#define REGS_PMC	((off_t)0x0000)
#define REGLEN_PMC	((size_t)0x2000)
#define REGS_PTIMER	((off_t)0x9000)
#define REGLEN_PTIMER	((size_t)0x1000)

// Reverse-engineered from strace and binary analysis.
typedef enum {
	NV_VERCHECK	= 0xc0484600 + NV_ESC_CHECK_VERSION_STR,
	NV_ENVINFO	= 0xc0044600 + NV_ESC_ENV_INFO,
	NV_CARDINFO	= 0xc6004600 + NV_ESC_CARD_INFO,
	NV_FOURTH	= 0xc00c4622,
	NV_GPUINVOKE	= 0xc020462a,
	NV_I6		= 0xc048464d,
	NV_I7		= 0xc014462d,
	NV_IA		= 0xc020462b,
	NV_IB		= 0xc030464e,
} nvioctls;

typedef enum {
	NV_D0		= 0xc0204637,
	NV_D1		= 0xc0144632,
} nvdevioctls;

// FIXME we'll almost certainly need a rwlock protecting this
static int nvctl = -1;

typedef struct nvhandshake {
	uint32_t ob[18];	// 0x48 bytes
} nvhandshake;

static int cardcount;
typedef struct {
	nv_ioctl_card_info_t descs[MAX_CARDS];
} thirdtype;

typedef struct fourthtype {
	uint32_t ob[3];		// 0xc (12) bytes
} fourthtype;

typedef struct gpuinvoke {	// 0x20 (32) bytes
	uint32_t obj;
	uint32_t fam;
	uint32_t meth;
	uint32_t ob;		// zero out
	uint64_t addr;
	uint32_t len;
	uint32_t ret;		// zero out on input
} gpuinvoke;

typedef struct typeb {
	uint32_t ob[12];	// 0x30 (48) bytes
} typeb;

typedef struct type6 {
	uint32_t ob[18];	// 0x30 (72) bytes
} type6;

typedef struct typed0 {
	uint32_t ob[8];		// 0x20 (32) bytes
} typed0;

static thirdtype t3;
static nv_ioctl_env_info_t envinfo;

static int
create_ctldev(const char *fp){
	mode_t mode,oldmask;
	dev_t dev;

	mode = NVCTLDEV_MODE;
	dev = makedev(NVCTLDEV_MAJOR,NVCTLDEV_MINOR);
	oldmask = umask(0);
	if(mknod(fp,mode,dev)){
		umask(oldmask);
		fprintf(stderr,"Couldn't create %s (%s)\n",fp,strerror(errno));
		return -1;
	}
	umask(oldmask);
	return 0;
}

static int
create_carddev(const char *fp,unsigned z){
	mode_t mode,oldmask;
	dev_t dev;

	if(z >= MAX_CARDS){
		fprintf(stderr,"Only up through %u cards are supported\n",MAX_CARDS);
		return -1;
	}
	mode = NVCTLDEV_MODE;
	dev = makedev(NVCTLDEV_MAJOR,z);
	oldmask = umask(0);
	if(mknod(fp,mode,dev)){
		umask(oldmask);
		fprintf(stderr,"Couldn't create %s (%s)\n",fp,strerror(errno));
		return -1;
	}
	umask(oldmask);
	return 0;
}

static int
invokegpu(int ctlfd,uint32_t fam,uint32_t meth,void *v,size_t vlen){
	gpuinvoke gpu;

	memset(&gpu,0,sizeof(gpu));
	gpu.fam = fam;
	gpu.meth = meth;
	gpu.addr = (uint64_t)v;
	gpu.len = vlen;
	if(ioctl(ctlfd,NV_GPUINVOKE,&gpu)){
		fprintf(stderr,"Error invoking GPU on fd %d (%s)\n",ctlfd,strerror(errno));
		return -1;
	}
	if(gpu.ret){
		fprintf(stderr,"GPU returned error %u on fd %d\n",gpu.ret,ctlfd);
		return -1;
	}
	return 0;
}

static CUresult
init_dev(int ctlfd,unsigned dno,CUdevice_opaque *dev){
	char devn[strlen(DEVROOT) + 4];
	char name[NVNAMEMAX];
	uint32_t *map;
	typed0 td0;
	off_t off;
	int dfd;

	debug("Device #%u register base: 0x%08zxb @ 0x%08jx\n",dno,dev->regsize,dev->regaddr);
	memset(dev->attrs,0,sizeof(dev->attrs));
	off = dev->regaddr + REGS_PMC;
	debug("Device #%u PMC: 0x%zxb @ 0x%jx\n",dno,REGLEN_PMC,off);
	if(snprintf(devn,sizeof(devn),"%s%u",DEVROOT,dno) >= (int)sizeof(devn)){
		return CUDA_ERROR_INVALID_VALUE;
	}
	if((dfd = open(devn,O_RDWR)) < 0){
		if(errno == ENOENT){
			if(create_carddev(devn,dno)){
				return CUDA_ERROR_INVALID_DEVICE;
			}
			if((dfd = open(devn,O_RDWR)) < 0){
				fprintf(stderr,"Couldn't open %s (%s)\n",devn,strerror(errno));
				return CUDA_ERROR_INVALID_DEVICE;
			}
		}else{
			fprintf(stderr,"Couldn't open %s (%s)\n",devn,strerror(errno));
			return CUDA_ERROR_INVALID_DEVICE;
		}
	}
	debug("Device #%u handle (%s) at fd %d\n",dno,devn,dfd);
	if((map = mmap(NULL,REGLEN_PMC,PROT_READ,MAP_SHARED,dfd,off)) == MAP_FAILED){
		fprintf(stderr,"Couldn't map PMC (%s); check dmesg\n",strerror(errno));
		close(dfd);
		return CUDA_ERROR_INVALID_DEVICE;
	}
	dev->arch = ((map[0] >> 20u) & 0xffu);
	dev->stepping = map[0] & 0xffu;
	// http://nouveau.freedesktop.org/wiki/CodeNames
	debug("Architecture: G%2X %2X\n",dev->arch,dev->stepping);
	memset(name,0,sizeof(name));
	if(invokegpu(ctlfd,0x5c000002,0x20800110,name,sizeof(name))){
		munmap(map,REGLEN_PMC);
		close(dfd);
		return CUDA_ERROR_INVALID_DEVICE;
	}
	strncpy(dev->name,name + 4,sizeof(dev->name));
	td0.ob[0] = 3251636241;
	td0.ob[1] = 3251636241;
	td0.ob[2] = 1;
	td0.ob[3] = 0;
	td0.ob[4] = 0;
	if(ioctl(dfd,NV_D0,&td0)){
		fprintf(stderr,"Error sending ioctl 0x%x to fd %d (%s)\n",NV_D0,dfd,strerror(errno));
		munmap(map,REGLEN_PMC);
		close(dfd);
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if(ioctl(dfd,NV_D0,&td0)){
		fprintf(stderr,"Error sending ioctl 0x%x to fd %d (%s)\n",NV_D0,dfd,strerror(errno));
		munmap(map,REGLEN_PMC);
		close(dfd);
		return CUDA_ERROR_INVALID_DEVICE;
	}
	munmap(map,REGLEN_PMC);
	close(dfd);
	return CUDA_SUCCESS;
}

static const char *
busname(unsigned bustype){
	switch(bustype){
	case NV_IOCTL_CARD_INFO_BUS_TYPE_PCI:
		return "PCI";
	case NV_IOCTL_CARD_INFO_BUS_TYPE_AGP:
		return "AGP";
	case NV_IOCTL_CARD_INFO_BUS_TYPE_PCI_EXPRESS:
		return "PCIe";
	default:
		return NULL;
	}
}

// For now, maxcds must equal MAX_CARDS FIXME
static int
get_card_count(int fd,int *count,CUdevice_opaque *devs,
		nv_ioctl_card_info_t *cds,unsigned maxcds){
	debug("Probing for up to %u cards\n",maxcds);
	*count = 0;
	memset(cds,0xff,maxcds / CHAR_BIT); // FIXME how does mask work?
	if(ioctl(fd,NV_CARDINFO,cds)){
		fprintf(stderr,"Error getting card info on fd %d (%s)\n",fd,strerror(errno));
		return -1;
	}
	while(maxcds--){
		if(cds[maxcds].flags & NV_IOCTL_CARD_INFO_FLAG_PRESENT){
			CUdevice_opaque *d;
			const char *bus;

			d = &devs[(*count)++];
			d->irq = cds[maxcds].interrupt_line;
			d->regaddr = cds[maxcds].reg_address;
			d->fbaddr = cds[maxcds].fb_address;
			d->regsize = cds[maxcds].reg_size;
			d->fbsize = cds[maxcds].fb_size;
			d->pcidomain = cds[maxcds].domain;
			d->busnumber = cds[maxcds].bus;
			d->flags = cds[maxcds].flags;
			d->slot = cds[maxcds].slot;
			d->vendorid = cds[maxcds].vendor_id;
			d->deviceid = cds[maxcds].device_id;
			debug("Found a device (%u total), ID #%u (IRQ %u)\n",
					*count,maxcds,d->irq);
			if((bus = busname(d->busnumber)) == NULL){
				fprintf(stderr,"Warning: unknown bus type %u\n",
						d->busnumber);
				debug("Domain: %u Slot: %u Bus: unknown\n",
					d->pcidomain,devs[maxcds].slot);
			}else{
				debug("Domain: %u Slot: %u Bus: %s\n",
					d->pcidomain,devs[maxcds].slot,bus);
			}
			debug("Vendor ID: 0x%04x Device ID: 0x%04x\n",
				d->vendorid,devs[maxcds].deviceid);
			debug("Flags: 0x%04x\n",d->flags);
			debug("Framebuffer: 0x%zx @ 0x%jx\n",
					d->fbsize,devs[maxcds].fbaddr);
		}
	}
	printf("Found %d cards\n",*count);
	return 0;
}

static int
convert_version(uint32_t *ob3,const char *verstr){
	unsigned z;

	debug("Expecting version '%s'\n",verstr);
	for(z = 0 ; z < 3 ; ++z){
		unsigned y,shl = 1;

		ob3[z] = 0;
		for(y = 0 ; y < sizeof(*ob3) && *verstr ; ++y, ++verstr){
			if((*verstr < '0' || *verstr > '9') && *verstr != '.'){
				return -1;
			}
			ob3[z] += (*(unsigned const char *)verstr) * shl;
			shl <<= 8u;
		}
	}
	if(*verstr){
		return -1;
	}
	return 0;
}

static CUresult
init_ctlfd(int fd,const char *ver){
	nvhandshake hshake;
	CUresult r;
	int z;

	memset(&hshake,0,sizeof(hshake));
	if(convert_version(hshake.ob + 2,ver)){
		fprintf(stderr,"Bad version: \"%s\"\n",ver);
		return CUDA_ERROR_INVALID_VALUE;
	}
	if(ioctl(fd,NV_VERCHECK,&hshake)){
		fprintf(stderr,"Error checking version on fd %d (%s)\n",fd,strerror(errno));
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if(hshake.ob[1] != NV_RM_API_VERSION_REPLY_RECOGNIZED){
		fprintf(stderr,"Version rejected; check dmesg (got 0x%x)\n",hshake.ob[0]);
		return CUDA_ERROR_INVALID_VALUE;
	}
	printf("Verified version %s\n",ver);
	memset(&envinfo,0,sizeof(envinfo));
	if(ioctl(fd,NV_ENVINFO,&envinfo)){
		fprintf(stderr,"Error checking PATs on fd %d (%s)\n",fd,strerror(errno));
		return CUDA_ERROR_INVALID_DEVICE;
	}
	debug("PAT support: %s\n",envinfo.pat_supported ? "yes" : "no");
	if(get_card_count(fd,&cardcount,devs,t3.descs,sizeof(devs) / sizeof(*devs))){
		return CUDA_ERROR_INVALID_DEVICE;
	}
	r = CUDA_SUCCESS;
	for(z = 0 ; z < cardcount ; ++z){
		CUdevice_opaque *dev = &devs[z];
		CUresult rr;

		if((rr = init_dev(fd,z,dev)) != CUDA_SUCCESS){
			r = rr;
		}
	}
	return r;
}

CUresult cuInit(unsigned flags){
	CUresult r;
	int fd;

	if(flags){
		return CUDA_ERROR_INVALID_VALUE;
	}
	if((fd = open(NVCTLDEV,O_RDWR)) < 0){
		if(errno == ENOENT){
		       	if(create_ctldev("/") == 0){
				return CUDA_ERROR_INVALID_DEVICE;
			}
			if((fd = open(NVCTLDEV,O_RDWR)) < 0){
				fprintf(stderr,"Couldn't open %s (%s)\n",NVCTLDEV,strerror(errno));
				return CUDA_ERROR_INVALID_DEVICE;
			}
		}else{
			fprintf(stderr,"Couldn't open %s (%s)\n",NVCTLDEV,strerror(errno));
			return CUDA_ERROR_INVALID_DEVICE;
		}
	}
	debug("CTL handle (%s) at fd %d\n",NVCTLDEV,fd);
	if((r = init_ctlfd(fd,CUDARUNVER)) != CUDA_SUCCESS){
		close(fd);
		return r;
	}
	if(nvctl >= 0){
		debug("Resetting old nvctl (%d) to new (%d)\n",nvctl,fd);
		close(nvctl);
	}
	nvctl = fd;
	return CUDA_SUCCESS;
}

CUresult cuDeviceGet(CUdevice *d,int devno){
	if(devno < 0 || (unsigned)devno >= sizeof(devs) / sizeof(*devs)){
		return CUDA_ERROR_INVALID_VALUE;
	}
	*d = &devs[devno];
	return CUDA_SUCCESS;
}

CUresult cuDeviceGetCount(int *count){
	*count = cardcount;
	return CUDA_SUCCESS;
}

CUresult cuDeviceGetAttribute(int *attr,CUdevice_attribute spec,CUdevice dev){
	if(spec >= sizeof(dev->attrs) / sizeof(*dev->attrs)){
		return CUDA_ERROR_INVALID_VALUE;
	}
	*attr = dev->attrs[spec];
	return CUDA_SUCCESS;
}

CUresult cuDeviceGetName(char *buf,int bufsz,CUdevice c){
	if(!c->name[0]){
		return CUDA_ERROR_NOT_INITIALIZED;
	}
	if(bufsz <= 0){
		return CUDA_ERROR_INVALID_VALUE;
	}
	strncpy(buf,c->name,bufsz);
	return CUDA_SUCCESS; // FIXME error on buf too small?
}
