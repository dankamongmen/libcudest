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

// 195.36.15, 195.36.24, 256.22
#define CUDARUNVER	"256.29"

#define DEVROOT "/dev/nvidia"
#define NVCTLDEV "/dev/nvidiactl"

#define MAX_CARDS 32 // FIXME pull from nv somehow? upstream constant

#define debug(s,...) fprintf(stderr,"%s:%d] "s,__func__,__LINE__,__VA_ARGS__)

typedef struct CUdevice_opaque {
	int valid;
	size_t regsize;
	uintmax_t regaddr;
	unsigned flags;
	unsigned irq;
	unsigned vendorid,deviceid,gpuid;
	unsigned pcidomain,busnumber,slot;
	int attrs[CU_DEVICE_ATTRIBUTE_ECC_ENABLED + 1];
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
	NV_FIFTH	= 0xc020462a,
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

typedef struct type5 {
	uint32_t ob[8];		// 0x20 (32) bytes
} type5;

typedef struct typeb {
	uint32_t ob[12];	// 0x30 (48) bytes
} typeb;

typedef struct type6 {
	uint32_t ob[18];	// 0x30 (72) bytes
} type6;

typedef struct typed0 {
	uint32_t ob[8];		// 0x20 (32) bytes
} typed0;

static type6 t6;
static thirdtype t3;
static fourthtype t4;
static type5 t5,ta,t7;
static nv_ioctl_env_info_t envinfo;

static CUresult
init_dev(unsigned dno,CUdevice_opaque *dev){
	char devn[strlen(DEVROOT) + 4];
	typed0 td0;
	void *map;
	off_t off;
	int dfd;

	debug("Device #%u register base: %zub @ 0x%jx\n",dno,dev->regsize,dev->regaddr);
	memset(dev->attrs,0,sizeof(dev->attrs));
	off = dev->regaddr + REGS_PMC;
	debug("Device #%u PMC: %zub @ 0x%jx\n",dno,REGLEN_PMC,off);
	if(snprintf(devn,sizeof(devn),"%s%u",DEVROOT,dno) >= (int)sizeof(devn)){
		return CUDA_ERROR_INVALID_VALUE;
	}
	if((dfd = open(devn,O_RDWR)) < 0){
		fprintf(stderr,"Couldn't open %s (%s)\n",devn,strerror(errno));
		return CUDA_ERROR_INVALID_DEVICE;
	}
	debug("Device #%u handle (%s) at fd %d\n",dno,devn,dfd);
	if((map = mmap(NULL,REGLEN_PMC,PROT_READ,MAP_SHARED,dfd,off)) == MAP_FAILED){
		fprintf(stderr,"Couldn't map PMC (%s); check dmesg\n",strerror(errno));
		close(dfd);
		return CUDA_ERROR_INVALID_DEVICE;
	}
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
			const char *bus;

			devs[maxcds].regaddr = cds[maxcds].reg_address;
			devs[maxcds].irq = cds[maxcds].interrupt_line;
			devs[maxcds].regsize = cds[maxcds].reg_size;
			devs[maxcds].pcidomain = cds[maxcds].domain;
			devs[maxcds].busnumber = cds[maxcds].bus;
			devs[maxcds].flags = cds[maxcds].flags;
			devs[maxcds].slot = cds[maxcds].slot;
			devs[maxcds].valid = 1;
			++*count;
			if((bus = busname(devs[maxcds].busnumber)) == NULL){
				fprintf(stderr,"Unknown bus type: %u\n",devs[maxcds].busnumber);
				return -1;
			}
			debug("Found a device (%u total), ID #%u (IRQ %u)\n",
					*count,maxcds,devs[maxcds].irq);
			debug("Flags: 0x%x\n",devs[maxcds].flags);
			debug("Bus: %s Domain: %u Slot: %u\n",bus,
				devs[maxcds].pcidomain,devs[maxcds].slot);
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
	/*
	if(ioctl(fd,NV_FOURTH,&t4)){
		fprintf(stderr,"Error sending ioctl 0x%x to fd %d (%s)\n",NV_FOURTH,fd,strerror(errno));
		return CUDA_ERROR_INVALID_DEVICE;
	}
	*/
	t5.ob[0] = t4.ob[0];
	t5.ob[1] = t4.ob[0];
	t5.ob[2] = 0x215u;
	t5.ob[4] = 0xcf43fd00u;
	t5.ob[5] = 0x00007fffu;
	t5.ob[6] = 0x84u;
	if(ioctl(fd,NV_FIFTH,&t5)){
		return CUDA_ERROR_INVALID_DEVICE;
	}
	t6.ob[0] = t4.ob[0];
	t6.ob[1] = t4.ob[0];
	t6.ob[2] = 0x1u;
	t6.ob[8] = 0xa14b1233u;
	t6.ob[9] = 0x00007feeu;
	t6.ob[10] = 0xfu;
	if(ioctl(fd,NV_I6,&t6)){
		return CUDA_ERROR_INVALID_DEVICE;
	}
	t5.ob[2] = 0x215u;
	t5.ob[3] = 0x0;
	t5.ob[6] = 0x80u;
	t5.ob[7] = 0x0;
	if(ioctl(fd,NV_FIFTH,&t5)){
		return CUDA_ERROR_INVALID_DEVICE;
	}
	t5.ob[4] = 0xcf43fac0;
	if(ioctl(fd,NV_FIFTH,&t5)){
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if(ioctl(fd,NV_FIFTH,&t5)){
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if(ioctl(fd,NV_IA,&ta)){
		return CUDA_ERROR_INVALID_DEVICE;
	}
	for(z = 0 ; z < cardcount ; ++z){ // FIXME what if non-contiguous?
		CUdevice_opaque *dev = &devs[z];

		if(!dev->valid){
			continue;
		}
		if((r = init_dev(z,dev)) != CUDA_SUCCESS){
			return r;
		}
	}
	if(ioctl(fd,NV_FIFTH,&t5)){
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if(ioctl(fd,NV_FIFTH,&t5)){
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if(ioctl(fd,NV_FIFTH,&t5)){
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if(ioctl(fd,NV_FIFTH,&t5)){
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if(ioctl(fd,NV_I7,&t7)){
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if(ioctl(fd,NV_FIFTH,&t5)){
		return CUDA_ERROR_INVALID_DEVICE;
	}
	return CUDA_SUCCESS;
}

CUresult cuInit(unsigned flags){
	CUresult r;
	int fd;

	if(flags){
		return CUDA_ERROR_INVALID_VALUE;
	}
	if((fd = open(NVCTLDEV,O_RDWR)) < 0){
		fprintf(stderr,"Couldn't open %s (%s)\n",NVCTLDEV,strerror(errno));
		return CUDA_ERROR_INVALID_DEVICE;
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
