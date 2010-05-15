#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <cudest.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/mman.h>

#define CUDAMAJMIN(v) v / 1000, v % 1000

int init_cuda_alldevs(int *count){
	int attr,cerr;

	if((cerr = cuInit(0)) != CUDA_SUCCESS){
		fprintf(stderr,"Couldn't initialize CUDA (%d)\n",cerr);
		return -1;
	}
	if((cerr = cuDriverGetVersion(&attr)) != CUDA_SUCCESS){
		fprintf(stderr,"Couldn't get CUDA driver version (%d)\n",cerr);
		return -1;
	}
	printf("Compiled against CUDA version %d.%d. Linked against CUDA version %d.%d.\n",
			CUDAMAJMIN(CUDA_VERSION),CUDAMAJMIN(attr));
	if(CUDA_VERSION > attr){
		fprintf(stderr,"Compiled against a newer version of CUDA than that installed, exiting.\n");
		return -1;
	}
	if((cerr = cuDeviceGetCount(count)) != CUDA_SUCCESS){
		fprintf(stderr,"Couldn't get CUDA device count (%d)\n",cerr);
		return -1;
	}
	if(*count <= 0){
		fprintf(stderr,"No CUDA devices found, exiting.\n");
		return -1;
	}
	return 0;
}

// CUDA must already have been initialized before calling cudaid().
#define CUDASTRLEN 80
static int
id_cuda(int dev,unsigned *mem,unsigned *tmem,int *state){
	int major,minor,attr,cerr,integrated;
	void *str = NULL;
	CUcontext ctx;
	CUdevice c;

	*state = 0;
	if((cerr = cuDeviceGet(&c,dev)) != CUDA_SUCCESS){
		fprintf(stderr," Couldn't associative with device (%d)\n",cerr);
		return cerr;
	}
	cerr = cuDeviceGetAttribute(&attr,CU_DEVICE_ATTRIBUTE_COMPUTE_MODE,c);
	if(cerr != CUDA_SUCCESS || attr <= 0){
		fprintf(stderr,"Error acquiring attribute %d (%d)\n",CU_DEVICE_ATTRIBUTE_COMPUTE_MODE,cerr);
		return cerr;
	}
	*state = attr;
	cerr = cuDeviceGetAttribute(&attr,CU_DEVICE_ATTRIBUTE_INTEGRATED,c);
	if(cerr != CUDA_SUCCESS || attr <= 0){
		fprintf(stderr,"Error acquiring attribute %d (%d)\n",CU_DEVICE_ATTRIBUTE_INTEGRATED,cerr);
		return cerr;
	}
	integrated = attr;
	cerr = cuDeviceGetAttribute(&attr,CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT,c);
	if(cerr != CUDA_SUCCESS || attr <= 0){
		fprintf(stderr,"Error acquiring attribute %d (%d)\n",CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT,cerr);
		return cerr;
	}
	if((cerr = cuDeviceComputeCapability(&major,&minor,c)) != CUDA_SUCCESS){
		fprintf(stderr,"Error determining compute capability (%d)\n",cerr);
		return cerr;
	}
	if((str = malloc(CUDASTRLEN)) == NULL){
		return -1;
	}
	if((cerr = cuDeviceGetName((char *)str,CUDASTRLEN,c)) != CUDA_SUCCESS){
		fprintf(stderr,"Error determining device name (%d)\n",cerr);
		goto err;
	}
	if((cerr = cuCtxCreate(&ctx,CU_CTX_MAP_HOST|CU_CTX_SCHED_YIELD,c)) != CUDA_SUCCESS){
		fprintf(stderr," Couldn't create context (%d)\n",cerr);
		goto err;
	}
	if((cerr = cuMemGetInfo(mem,tmem)) != CUDA_SUCCESS){
		fprintf(stderr,"Error getting memory info (%d)\n",cerr);
		cuCtxDetach(ctx);
		goto err;
	}
	if(printf("%d.%d %s %s %u/%uMB free %s\n",
		major,minor,
		integrated ? "Integrated" : "Standalone",(char *)str,
		*mem / (1024 * 1024) + !!(*mem / (1024 * 1024)),
		*tmem / (1024 * 1024) + !!(*tmem / (1024 * 1024)),
		*state == CU_COMPUTEMODE_EXCLUSIVE ? "(exclusive)" :
		*state == CU_COMPUTEMODE_PROHIBITED ? "(prohibited)" :
		*state == CU_COMPUTEMODE_DEFAULT ? "(shared)" :
		"(unknown compute mode)") < 0){
		cerr = -1;
		goto err;
	}
	free(str);
	return CUDA_SUCCESS;

err:	// cerr ought already be set!
	free(str);
	return cerr;
}

static void
usage(const char *a0,int status){
	fprintf(stderr,"usage: %s\n",a0);
	exit(status);
}

int main(int argc,char **argv){
	int z,count;

	if(argc > 1){
		usage(argv[0],EXIT_FAILURE);
	}
	if(init_cuda_alldevs(&count)){
		return EXIT_FAILURE;
	}
	printf("CUDA device count: %d\n",count);
	for(z = 0 ; z < count ; ++z){
		unsigned mem,tmem;
		int state;

		printf(" %03d ",z);
		if(id_cuda(z,&mem,&tmem,&state)){
			return EXIT_FAILURE;
		}
		if(state != CU_COMPUTEMODE_DEFAULT){
			printf("  Skipping device %d (put it in shared mode).\n",z);
			continue;
		}
	}
	return EXIT_SUCCESS;
}
