#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>

// This'll at least allow ANSI coloring, etc
typedef struct {
	const char *boldon;
	const char *normal;
} ui;

static const ui ansi_ui = {
	.boldon = "\x1b[1m",
	.normal = "\x1b[0m",
},null_ui = {
	.boldon = "",
	.normal = "",
};

static const ui *curui = &ansi_ui;

static void
disable_ansi_check(FILE *fp){
	if(!isatty(fileno(fp))){
		curui = &null_ui;
	}
}

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
		disable_ansi_check(stdout);
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

static void
dump_mem(const uint32_t *dat,size_t s){
	size_t z;

	for(z = 0 ; z < s ; z += 4){
		printf("\x1b[1m");
		if(z % 16 == 0 && z){
			printf("0x%04zx\t\t\t\t",z);
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

static void
decode_gpucall_pre(const uint32_t *dat,size_t s,size_t *ps){
	if(s != 32){
		fprintf(stderr,"Invalid argument for GPU invocation (%zub)\n",s);
		return;
	}
	printf("GPU method 0x%08x:%08x\t",dat[1],dat[2]);
	*ps = ((uint64_t)dat[7] << 32) + dat[6];
	dump_mem((uint32_t *)(((uint64_t)dat[5] << 32) + dat[4]),*ps);
}

static void
decode_gpucall_post(const uint32_t *dat,size_t s,size_t ps){
	if(s != 32){
		fprintf(stderr,"Invalid argument for GPU invocation (%zub)\n",s);
		return;
	}
	if(ps != ((uint64_t)dat[7] << 32) + dat[6]){
		printf("\x1b[1m\x1b[44mLENGTH MODIFIED BY CALL!\t(ret: 0x%x)\x1b[0m\n",dat[7]);
	}
	printf("GPU method 0x%08x:%08x\t",dat[1],dat[2]);
	dump_mem((uint32_t *)(((uint64_t)dat[5] << 32) + dat[4]),ps);
}

int ioctl(int fd,int req,uintptr_t op){//,unsigned o1,unsigned o2){
	static int (*shim_ioctl)(int,int,uintptr_t,int,int);
	static int (*shim_ioctl3)(int,int,uintptr_t);
	const uint32_t *dat = (const uint32_t *)op;
	size_t gpus;
	int r,s;

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
		disable_ansi_check(stdout);
	}
	s = (req >> 16u) & 0x3fff;
	printf("ioctl %x, %d-byte param, fd %d\t",req & 0xff,s,fd);
	dump_mem(dat,s);

	if((req & 0xffu) == 0x2au){
		decode_gpucall_pre(dat,s,&gpus);
	}

	r = shim_ioctl3(fd,req,op);
	printf("\x1b[1m\x1b[34mRESULT: %d\x1b[0m%s",r,r ? "\n" : "\t\t\t");
	if(r == 0){
		dump_mem(dat,s);
		if((req & 0xffu) == 0x2a){
			decode_gpucall_post(dat,s,gpus);
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
		disable_ansi_check(stdout);
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
		disable_ansi_check(stdout);
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
		disable_ansi_check(stdout);
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
		disable_ansi_check(stdout);
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
		disable_ansi_check(stdout);
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
		disable_ansi_check(stdout);
	}
	printf("getenv(\x1b[1m%s\x1b[0m) = ",name);
	if( (r = shim_getenv(name)) ){
		printf("\x1b[1m\"%s\"\x1b[0m\n",r);
	}else{
		printf("\x1b[1mNULL\x1b[0m\n");
	}
	return r;
}
