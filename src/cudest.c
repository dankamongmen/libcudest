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
#include <sys/stat.h>
#include <sys/ioctl.h>

// 195.36.15, 195.36.24, 256.22, 256.29, 256.35, 260.19.29, 275.36
#define CUDARUNVER	"275.36"

#define DEVROOT "/dev/nvidia"
#define NVCTLDEV "/dev/nvidiactl"
#define NVCTLDEV_MODE (S_IFCHR | (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))
#define NV_CONTROL_DEVICE_MINOR 255
#define NVNAMEMAX 0x84		// from the "get name" GPU method paramlen

#define debug(s,...) fprintf(stderr,"%s:%d] "s,__func__,__LINE__, ##__VA_ARGS__)

typedef struct CUdevice_opaque {
	size_t regsize,fbsize;
	uintmax_t regaddr,fbaddr;
	unsigned arch;
	unsigned flags;
	unsigned irq;
	unsigned pcidomain,bus,slot;
	unsigned vendorid,deviceid,gpuid,pcirev;
	int attrs[CU_DEVICE_ATTRIBUTE_PCI_DEVICE_ID + 1];
	char name[NVNAMEMAX];
} CUdevice_opaque;

static CUdevice_opaque devs[NV_MAX_DEVICES];

// http://nouveau.freedesktop.org/wiki/HwIntroduction
#define BAR0_PMC	((off_t)0x0)
#define BAR0_PMC_LEN	((size_t)0x2000)
#define BAR0_PBUS	((off_t)0x88000)
#define BAR0_PBUS_LEN	((size_t)0x1000)
#define BAR0_PTIMER	((off_t)0x9000)
#define BAR0_PTIMER_LEN	((size_t)0x1000)

// Reverse-engineered from strace and binary analysis.
typedef enum {
	NV_VERCHECK	= 0xc0484600 + NV_ESC_CHECK_VERSION_STR,
	NV_ENVINFO	= 0xc0044600 + NV_ESC_ENV_INFO,
	NV_CARDINFO	= 0xc6004600 + NV_ESC_CARD_INFO,
	NV_FOURTH	= 0xc00c4622,
	NV_GPUINVOKE	= 0xc020462a,
	NV_GPUOBJ	= 0xc020462b,
	NV_I6		= 0xc048464d,
	NV_I7		= 0xc014462d,
	NV_IB		= 0xc030464e,
} nvioctls;

typedef enum {
	NV_D0		= 0xc0204637,
	NV_D1		= 0xc0144632,
} nvdevioctls;

// GPU objects, prepared via the NV_GPUOBJ ioctl -------------------------
typedef struct gpucontext {
	uint32_t ob[14];	// 0x38 (56) bytes
} gpucontext;
// -----------------------------------------------------------------------

// FIXME we'll almost certainly need a rwlock protecting this
static int nvctl = -1;

typedef struct nvhandshake {
	uint32_t ob[18];	// 0x48 (72) bytes
} nvhandshake;

typedef struct gpuobject {
	uint32_t ob[15];	// 0x3C (60) bytes
} gpuobject;

static int cardcount;
typedef struct nvcardinfo {
	nv_ioctl_card_info_t descs[NV_MAX_DEVICES];
} nvcardinfo;

typedef struct gpuinvoke {	// 0x20 (32) bytes
	uint32_t obj;
	uint32_t fam;
	uint32_t meth;
	uint32_t ob;		// zero out
	uint64_t addr;
	uint32_t len;
	uint32_t ret;		// zero out on input
} gpuinvoke;

static nvcardinfo t3;
static nv_ioctl_env_info_t envinfo;

static int
create_ctldev(const char *fp){
	mode_t mode,oldmask;
	dev_t dev;

	mode = NVCTLDEV_MODE;
	dev = makedev(NV_MAJOR_DEVICE_NUMBER,NV_CONTROL_DEVICE_MINOR);
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

	if(z >= NV_MAX_DEVICES){
		fprintf(stderr,"Only up through %u cards are supported\n",NV_MAX_DEVICES);
		return -1;
	}
	mode = NVCTLDEV_MODE;
	dev = makedev(NV_MAJOR_DEVICE_NUMBER,z);
	oldmask = umask(0);
	if(mknod(fp,mode,dev)){
		umask(oldmask);
		fprintf(stderr,"Couldn't create %s (%s)\n",fp,strerror(errno));
		return -1;
	}
	umask(oldmask);
	return 0;
}

/*static int
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
}*/

static CUresult
init_dev(int ctlfd,unsigned dno,CUdevice_opaque *dev){
	char devn[strlen(DEVROOT) + 4];
	gpuobject contextreq;
	gpucontext *context;
	uint32_t *map;
	size_t mlen;
	off_t off;
	int dfd;

	memset(dev->attrs,0,sizeof(dev->attrs));
	off = dev->regaddr + BAR0_PMC;
	mlen = BAR0_PMC_LEN;
	debug("Device #%u PMC: 0x%zxb @ 0x%jx\n",dno,mlen,(uintmax_t)off);
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
	if((map = mmap(NULL,mlen,PROT_READ,MAP_SHARED,dfd,off)) == MAP_FAILED){
		fprintf(stderr,"Couldn't map PMC (%s); check dmesg\n",strerror(errno));
		close(dfd);
		return CUDA_ERROR_INVALID_DEVICE;
	}
	// Architecture and stepping are in PMC_BOOT_0
	dev->arch = ((map[0] >> 20u) & 0xffu);
	// http://nouveau.freedesktop.org/wiki/CodeNames
	debug("Architecture: G%2X Stepping: %2X\t(0x%08x)\n",dev->arch,map[0] & 0xffu,map[0]);
	// http://nouveau.freedesktop.org/wiki/HwIntroduction
	debug("Endianness: %s-endian\t\t(0x%08x)\n",map[1] ? "big" : "little",map[1]);
	debug("PRAMIN maps physaddr: %x\t(0x%08x)\n",map[1472] << 16u,map[1472]);
	debug("PMC_ENABLE: 0x%08x\n",map[128]);
	debug("MPS_ENABLE: 0x%08x\n",map[1360]);
	if(munmap(map,mlen)){
		close(dfd);
		return CUDA_ERROR_INVALID_DEVICE;
	}
	mlen = BAR0_PBUS_LEN;
	off = dev->regaddr + BAR0_PBUS;
	debug("Device #%u PBUS: 0x%zxb @ 0x%jx\n",dno,mlen,(uintmax_t)off);
	if((map = mmap(NULL,mlen,PROT_READ,MAP_SHARED,dfd,off)) == MAP_FAILED){
		fprintf(stderr,"Couldn't map PBUS (%s); check dmesg\n",strerror(errno));
		close(dfd);
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if(dev->vendorid != (map[0] & 0xffffu)){
		fprintf(stderr,"Warning: reported vendor ID != PBUS (0x%08x)\n",
				(map[0] & 0xffffu));
	}
	if((dev->deviceid << 16u) != (map[0] & 0xffff0000u)){
		fprintf(stderr,"Warning: reported device ID != PBUS (0x%08x)\n",
				(map[0] & 0xffff0000u));
	}
	debug("Confirmed expected vendor and device IDs on PBUS\n");
	dev->attrs[CU_DEVICE_ATTRIBUTE_PCI_DEVICE_ID] = dev->deviceid;
	dev->pcirev = map[2] & 0xffu;
	debug("PCI rev: %02x      class: %02x.%02x\t(0x%08x)\n",dev->pcirev,
		((map[2] & 0xff000000u) >> 24u),((map[2] & 0x00ff0000u) >> 16u),map[2]);
	if(munmap(map,mlen)){
		close(dfd);
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if(close(dfd)){
		return CUDA_ERROR_INVALID_DEVICE;
	}

	if((context = malloc(sizeof(*context))) == NULL){
		fprintf(stderr,"Couldn't create GPU context (%s)\n",strerror(errno));
		return CUDA_ERROR_OUT_OF_MEMORY;
	}
	memset(&contextreq,0,sizeof(contextreq));
	//  FIXME
	((uintptr_t *)contextreq.ob)[2] = (uintptr_t)context;
	if(ioctl(ctlfd,NV_GPUOBJ,&contextreq)){
		fprintf(stderr,"Couldn't create GPU object (%s)\n",strerror(errno));
		free(context);
		return CUDA_ERROR_OUT_OF_MEMORY;
	}
	if(((uint64_t *)contextreq.ob)[3]){
		fprintf(stderr,"GPU returned error on context creation\n");
		free(context);
		return CUDA_ERROR_OUT_OF_MEMORY;
	}
	printf("Got a context\n");

	/*
	char name[NVNAMEMAX];
	// FIXME need a context
	memset(name,0,sizeof(name));
	if(invokegpu(ctlfd,0x5c000002,0x20800110,name,sizeof(name))){
		return CUDA_ERROR_INVALID_DEVICE;
	}
	strncpy(dev->name,name + 4,sizeof(dev->name));*/
	return CUDA_SUCCESS;
}

static const char *
busname(unsigned bustype) __attribute__ ((unused));

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

// For now, maxcds must equal NV_MAX_CARDS FIXME
static int
get_card_count(int fd,int *count,CUdevice_opaque *devs,
		nv_ioctl_card_info_t *cds,unsigned maxcds){
	unsigned z;

	debug("Probing for up to %u cards\n",maxcds);
	*count = 0;
	memset(cds,0xff,maxcds / CHAR_BIT); // FIXME how does mask work?
	if(ioctl(fd,NV_CARDINFO,cds)){
		fprintf(stderr,"Error getting card info on fd %d (%s)\n",fd,strerror(errno));
		return -1;
	}
	for(z = 0 ; z < maxcds ; ++z){
		if(cds[z].flags & NV_IOCTL_CARD_INFO_FLAG_PRESENT){
			CUdevice_opaque *d;

			d = &devs[(*count)++];
			d->irq = cds[z].interrupt_line;
			d->regaddr = cds[z].reg_address;
			d->fbaddr = cds[z].fb_address;
			d->regsize = cds[z].reg_size;
			d->fbsize = cds[z].fb_size;
			d->pcidomain = cds[z].domain;
			d->bus = cds[z].bus;
			d->flags = cds[z].flags;
			d->slot = cds[z].slot;
			d->vendorid = cds[z].vendor_id;
			d->deviceid = cds[z].device_id;
			debug("Found device %u (IRQ %u)\n",*count,d->irq);
			debug("Domain: %u Bus: %u Slot: %u\n",
				d->pcidomain,devs[z].bus,devs[z].slot);
			debug("Vendor ID: 0x%04x Device ID: 0x%04x\n",
				d->vendorid,devs[z].deviceid);
			debug("Flags: 0x%04x\n",d->flags);
			debug("Framebuffer: 0x%zx @ 0x%jx\n",
					d->fbsize,devs[z].fbaddr);
			debug("Register base: 0x%08zxb @ 0x%08jx\n",
					d->regsize,d->regaddr);
		}
	}
	printf("Found %d card%s\n",*count,*count == 1 ? "" : "s");
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
		       	if(create_ctldev(NVCTLDEV)){
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
