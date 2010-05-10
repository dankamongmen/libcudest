#include "cudest.h"
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define DEVROOT "/dev/nvidia"
#define NVCTLDEV "/dev/nvidiactl"

// Reverse-engineered from strace and binary analysis.
typedef enum {
	NV_HANDSHAKE	= 0xc04846d2,
	NV_SECOND	= 0xc00446ca,
	NV_THIRD	= 0xc60046c8,
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
	uint64_t ob[9];	// 0x48 bytes
} nvhandshake;

typedef uint32_t secondtype;

typedef struct thirdtype {
	uint32_t ob[384];	// 1536 (0x600) bytes
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
static secondtype result0xca;

#define DEVMAP_SIZE ((size_t)0x1000)
#define DEVMAP_OFF ((off_t)0xf2009000)

static CUresult
init_dev(unsigned dno){
	char devn[strlen(DEVROOT) + 4];
	typed0 td0;
	void *map;
	int dfd;

	if(snprintf(devn,sizeof(devn),"%s%u",DEVROOT,dno) >= (int)sizeof(devn)){
		return CUDA_ERROR_INVALID_VALUE;
	}
	if((dfd = open(devn,O_RDWR)) < 0){
		fprintf(stderr,"Couldn't open %s (%s)\n",devn,strerror(errno));
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if((map = mmap(NULL,DEVMAP_SIZE,PROT_READ,MAP_SHARED,dfd,DEVMAP_OFF)) == MAP_FAILED){
		fprintf(stderr,"Couldn't mmap() %zx (%s)\n",DEVMAP_SIZE,strerror(errno));
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
		munmap(map,DEVMAP_SIZE);
		close(dfd);
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if(ioctl(dfd,NV_D0,&td0)){
		fprintf(stderr,"Error sending ioctl 0x%x to fd %d (%s)\n",NV_D0,dfd,strerror(errno));
		munmap(map,DEVMAP_SIZE);
		close(dfd);
		return CUDA_ERROR_INVALID_DEVICE;
	}
	munmap(map,DEVMAP_SIZE);
	close(dfd);
	return CUDA_SUCCESS;
}

static CUresult
init_ctlfd(int fd){
	nvhandshake hshake;
	CUresult r;

	memset(&hshake,0,sizeof(hshake));
	//hshake.ob[2] = 0x35ull;		// 195.36.15
	//hshake.ob[1] = 0x312e36332e353931ull;	// 195.36.15
	hshake.ob[2] = 0x34ull;			// 195.36.24
	hshake.ob[1] = 0x322e36332e353931ull;	// 195.36.24
	if(ioctl(fd,NV_HANDSHAKE,&hshake)){
		fprintf(stderr,"Error sending ioctl 0x%x to fd %d (%s)\n",NV_HANDSHAKE,fd,strerror(errno));
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if(ioctl(fd,NV_SECOND,&result0xca)){
		fprintf(stderr,"Error sending ioctl 0x%x to fd %d (%s)\n",NV_SECOND,fd,strerror(errno));
		return CUDA_ERROR_INVALID_DEVICE;
	}
	t3.ob[0] = (uint32_t)-1;
	if(ioctl(fd,NV_THIRD,&t3)){
		fprintf(stderr,"Error sending ioctl 0x%x to fd %d (%s)\n",NV_THIRD,fd,strerror(errno));
		return CUDA_ERROR_INVALID_DEVICE;
	}
	if(ioctl(fd,NV_FOURTH,&t4)){
		fprintf(stderr,"Error sending ioctl 0x%x to fd %d (%s)\n",NV_FOURTH,fd,strerror(errno));
		return CUDA_ERROR_INVALID_DEVICE;
	}
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
	if((r = init_dev(0)) != CUDA_SUCCESS){
		return r;
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
	if((r = init_ctlfd(fd)) != CUDA_SUCCESS){
		close(fd);
		return r;
	}
	if(nvctl >= 0){
		close(nvctl);
	}
	nvctl = fd;
	return CUDA_SUCCESS;
}

CUresult cuDeviceGet(CUdevice *d,int devno){
	if(devno < 0){
		return CUDA_ERROR_INVALID_VALUE;
	}
	d->devno = devno;
	return CUDA_SUCCESS;
}
