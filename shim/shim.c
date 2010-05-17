#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>

void *mmap64(void *addr,size_t len,int prot,int flags,int fd,off_t off){
	static void *(*shim_mmap)(void *,size_t,int,int,int,off_t);
	unsigned *r;
	unsigned z;

	if(shim_mmap == NULL){
		const char *msg;

		fprintf(stderr,"shimming system's mmap(2)\n");
		if((shim_mmap = dlsym(RTLD_NEXT,"mmap")) == NULL){
			fprintf(stderr,"got a NULL mmap(2)\n");
			errno = EPERM;
			return MAP_FAILED;
		}
		if( (msg = dlerror()) ){
			fprintf(stderr,"couldn't shim mmap(2): %s\n",msg);
			errno = EPERM;
			return MAP_FAILED;
		}
	}
	printf("offset: 0x%jx\n",off);
	if(addr){
		printf("mmap 0x%zxb %d [%c%c%c%c] @ %p (%s)",len,fd,
				prot & PROT_READ ? 'R' : 'r',
				prot & PROT_WRITE ? 'W' : 'w',
				prot & PROT_EXEC ? 'X' : 'x',
				flags & MAP_PRIVATE ? 'p' : 's',
				addr,
				flags & MAP_FIXED ? "fixed" : "hint");
	}else{
		printf("mmap 0x%zxb %d [%c%c%c%c] (no hint)",len,fd,
				prot & PROT_READ ? 'R' : 'r',
				prot & PROT_WRITE ? 'W' : 'w',
				prot & PROT_EXEC ? 'X' : 'x',
				flags & MAP_PRIVATE ? 'P' : 'S');
	}
	fflush(stdout);
	r = shim_mmap(addr,len,prot,flags,fd,off);
	if(r == MAP_FAILED){
		printf(" FAILED\n");
	}else if(flags & MAP_PRIVATE){
		printf(" %p\n",r);
	}else{
		printf("\t");
		for(z = 0 ; z < len ; z += 4){
			printf("\x1b[1m");
			if(z % 16 == 0 && z){
				printf("0x%04lx\t\t\t",(uintptr_t)r + z);
			}
			if(r[z / 4]){
				printf("\x1b[32m");
			}
			printf("0x%08x ",r[z / 4]);
			printf("\x1b[0m");
			if(z % 16 == 12){
				printf("\n");
			}
		}
	}
	printf("\n");
	return r;
}

void *mmap(void *addr,size_t len,int prot,int flags,int fd,off_t off){
	return mmap64(addr,len,prot,flags,fd,off);
}

int ioctl(int fd,int req,uintptr_t op){//,unsigned o1,unsigned o2){
	static int (*shim_ioctl)(int,int,uintptr_t,int,int);
	static int (*shim_ioctl3)(int,int,uintptr_t);
	const uint32_t *dat = (const uint32_t *)op;
	int r,s,z;

	if(shim_ioctl == NULL){
		const char *msg;

		fprintf(stderr,"shimming system's ioctl(2)\n");
		if((shim_ioctl = dlsym(RTLD_NEXT,"ioctl")) == NULL){
			fprintf(stderr,"got a NULL ioctl(2)\n");
			errno = EPERM;
			return -1;
		}
		if( (msg = dlerror()) ){
			fprintf(stderr,"couldn't shim ioctl(2): %s\n",msg);
			errno = EPERM;
			return -1;
		}
		if((shim_ioctl3 = dlsym(RTLD_NEXT,"ioctl")) == NULL){
			fprintf(stderr,"got a NULL ioctl(2)\n");
			errno = EPERM;
			return -1;
		}
		if( (msg = dlerror()) ){
			fprintf(stderr,"couldn't shim ioctl(2): %s\n",msg);
			errno = EPERM;
			return -1;
		}
	}
	s = (req >> 16u) & 0x3fff;
	printf("ioctl %x, %d-byte param, fd %d\t",req & 0xff,s,fd);
	for(z = 0 ; z < s ; z += 4){
		printf("\x1b[1m");
		if(z % 16 == 0 && z){
			printf("0x%04x\t\t\t\t",z);
		}
		if(dat[z / 4]){
			printf("\x1b[32m");
		}
		printf("0x%08x ",dat[z / 4]);
		printf("\x1b[0m");
		if(z % 16 == 12){
			printf("\n");
		}
	}
	if(z % 16){
		printf("\n");
	}
	r = shim_ioctl3(fd,req,op);
	printf("\x1b[1m\x1b[34mRESULT: %d\x1b[0m\t\t\t",r);
	if(r == 0){
		for(z = 0 ; z < s ; z += 4){
			printf("\x1b[1m");
			if(z % 16 == 0 && z){
				printf("0x%04x\t\t\t\t",z);
			}
			if(dat[z / 4]){
				printf("\x1b[32m");
			}
			printf("0x%08x ",dat[z / 4]);
			printf("\x1b[0m");
			if(z % 16 == 12){
				printf("\n");
			}
		}
		if(z % 16){
			printf("\n");
		}
	}
	printf("\n");
	return r;
}

int open64(const char *p,int flags,mode_t mode){
	static int (*shim_open)(const char *,int,mode_t);
	int r;

	if(shim_open == NULL){
		const char *msg;

		fprintf(stderr,"shimming system's open(2)\n");
		if((shim_open = dlsym(RTLD_NEXT,"open")) == NULL){
			fprintf(stderr,"got a NULL open(2)\n");
			return 0;
		}
		if( (msg = dlerror()) ){
			fprintf(stderr,"couldn't shim open(2): %s\n",msg);
			return 0;
		}
	}
	printf("open(\x1b[1m\"%s\", \"%d\"\x1b[0m) = ",p,flags);
	r = shim_open(p,flags,mode);
	printf("\x1b[1m%d\x1b[0m\n",r);
	return r;
}

int strcoll(const char *s0,const char *s1){
	static int (*shim_strcoll)(const char *,const char *);
	int r;

	if(shim_strcoll == NULL){
		const char *msg;

		fprintf(stderr,"shimming system's strcoll(2)\n");
		if((shim_strcoll = dlsym(RTLD_NEXT,"strcoll")) == NULL){
			fprintf(stderr,"got a NULL strcoll(2)\n");
			return 0;
		}
		if( (msg = dlerror()) ){
			fprintf(stderr,"couldn't shim strcoll(2): %s\n",msg);
			return 0;
		}
	}
	printf("strcoll(\x1b[1m\"%s\", \"%s\"\x1b[0m) = ",s0,s1);
	r = shim_strcoll(s0,s1);
	printf("\x1b[1m%d\x1b[0m\n",r);
	return r;
}

int strcmp(const char *s0,const char *s1){
	static int (*shim_strcmp)(const char *,const char *);
	int r;

	if(shim_strcmp == NULL){
		const char *msg;

		fprintf(stderr,"shimming system's strcmp(2)\n");
		if((shim_strcmp = dlsym(RTLD_NEXT,"strcmp")) == NULL){
			fprintf(stderr,"got a NULL strcmp(2)\n");
			return 0;
		}
		if( (msg = dlerror()) ){
			fprintf(stderr,"couldn't shim strcmp(2): %s\n",msg);
			return 0;
		}
	}
	printf("strcmp(\x1b[1m\"%s\", \"%s\"\x1b[0m) = ",s0,s1);
	r = shim_strcmp(s0,s1);
	printf("\x1b[1m%d\x1b[0m\n",r);
	return r;
}

int strncmp(const char *s0,const char *s1,size_t n){
	static int (*shim_strcmp)(const char *,const char *,size_t);
	int r;

	if(shim_strcmp == NULL){
		const char *msg;

		fprintf(stderr,"shimming system's strcmp(2)\n");
		if((shim_strcmp = dlsym(RTLD_NEXT,"strcmp")) == NULL){
			fprintf(stderr,"got a NULL strcmp(2)\n");
			return 0;
		}
		if( (msg = dlerror()) ){
			fprintf(stderr,"couldn't shim strcmp(2): %s\n",msg);
			return 0;
		}
	}
	printf("strcmp(\x1b[1m\"%s\", \"%s\", %zu\x1b[0m) = ",s0,s1,n);
	r = shim_strcmp(s0,s1,n);
	printf("\x1b[1m%d\x1b[0m\n",r);
	return r;
}

size_t strlen(const char *s){
	static size_t (*shim_strlen)(const char *);
	size_t r;

	if(shim_strlen == NULL){
		const char *msg;

		fprintf(stderr,"shimming system's strlen(2)\n");
		if((shim_strlen = dlsym(RTLD_NEXT,"strlen")) == NULL){
			fprintf(stderr,"got a NULL strlen(2)\n");
			return 0;
		}
		if( (msg = dlerror()) ){
			fprintf(stderr,"couldn't shim strlen(2): %s\n",msg);
			return 0;
		}
	}
	printf("strlen(\x1b[1m%s\x1b[0m) = ",s);
	r = shim_strlen(s);
	printf("\x1b[1m\"%zu\"\x1b[0m\n",r);
	return r;
}

char *getenv(const char *name){
	static char *(*shim_getenv)(const char *);
	char *r;

	if(shim_getenv == NULL){
		const char *msg;

		fprintf(stderr,"shimming system's getenv(2)\n");
		if((shim_getenv = dlsym(RTLD_NEXT,"getenv")) == NULL){
			fprintf(stderr,"got a NULL getenv(2)\n");
			return NULL;
		}
		if( (msg = dlerror()) ){
			fprintf(stderr,"couldn't shim getenv(2): %s\n",msg);
			return NULL;
		}
	}
	printf("getenv(\x1b[1m%s\x1b[0m) = ",name);
	if( (r = shim_getenv(name)) ){
		printf("\x1b[1m\"%s\"\x1b[0m\n",r);
	}else{
		printf("\x1b[1mNULL\x1b[0m\n");
	}
	return r;
}
