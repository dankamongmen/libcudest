#include <cudest.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

static void
usage(const char *argv){
	fprintf(stderr,"usage: %s\n",argv);
}

static int
get_all_attributes(CUdevice c){
	int attr,n;

	for(n = 0 ; n < CU_DEVICE_ATTRIBUTE_PCI_DEVICE_ID ; ++n){
		CUresult cerr;

		if( (cerr = cuDeviceGetAttribute(&attr,n,c)) ){
			fprintf(stderr,"Error acquiring device attr %d (%d)\n",n,cerr);
			return -1;
		}
		printf("Device attribute %d: %d\n",n,attr);
	}
	return 0;
}

int main(int argc,char **argv){
	int count,devno;
	//CUdeviceptr p;
	CUresult cerr;
	char str[80];
	CUdevice c;

	if(argc > 1){
		usage(*argv);
		exit(EXIT_FAILURE);
	}
	if( (cerr = cuInit(0)) ){
		fprintf(stderr,"Couldn't initialize CUDA (%d)\n",cerr);
		exit(EXIT_FAILURE);
	}
	printf("CUDA initialized.\n");
	if( (cerr = cuDeviceGetCount(&count)) ){
		fprintf(stderr,"Couldn't get device count (%d)\n",cerr);
		exit(EXIT_FAILURE);
	}else if(count == 0){
		fprintf(stderr,"Couldn't find any devices\n");
		exit(EXIT_FAILURE);
	}
	printf("We have %d device%s.\n",count,count == 1 ? "" : "s");
	for(devno = 0 ; devno < count ; ++devno){
		if( (cerr = cuDeviceGet(&c,devno)) ){
			fprintf(stderr,"Couldn't reference device %d (%d)\n",devno,cerr);
			exit(EXIT_FAILURE);
		}
		if(get_all_attributes(c)){
			exit(EXIT_FAILURE);
		}
		if( (cerr = cuDeviceGetName(str,sizeof(str),c)) ){
			fprintf(stderr,"Error determining device name (%d)\n",cerr);
			exit(EXIT_FAILURE);
		}
		printf("Device %d: %s\n",devno,str);
		/*
		if( (cerr = cuMemAlloc(&p,sizeof(p))) ){
			fprintf(stderr,"Couldn't allocate %zub (%d)\n",sizeof(p),cerr);
			exit(EXIT_FAILURE);
		}*/
	}
	exit(EXIT_SUCCESS);
}
