#include <cudest.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

// FIXME: we really ought take a bus specification rather than a device number,
// since the latter are unsafe across hardware removal/additions.
static void
usage(const char *a0){
	fprintf(stderr,"usage: %s devno\n",a0);
}

static int
getzul(const char *arg,unsigned long *zul){
	char *eptr;

	if(((*zul = strtoul(arg,&eptr,0)) == ULONG_MAX && errno == ERANGE)
			|| eptr == arg || *eptr){
		fprintf(stderr,"Expected an unsigned integer, got \"%s\"\n",arg);
		return -1;
	}
	return 0;
}

int main(int argc,char **argv){
	unsigned total = 0;
	unsigned long zul;
	CUdevice dev;
	CUcontext c;
	int cerr;

	if(argc != 2){
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	if(getzul(argv[1],&zul)){
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	if(cuInit(0)){
		fprintf(stderr,"Couldn't initialize cuda\n");
		exit(EXIT_FAILURE);
	}
	if(cuDeviceGet(&dev,zul)){
		fprintf(stderr,"Couldn't get device %lu\n",zul);
		exit(EXIT_FAILURE);
	}
	while((cerr = cuCtxCreate(&c,0,dev)) == CUDA_SUCCESS){
		++total;
	}
	printf("Context creation failed (%d).\n",cerr);
	return printf("Created %u contexts.\n",total) < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
