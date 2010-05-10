#ifndef CUDEST_CUDEST
#define CUDEST_CUDEST

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

// Taken from 3.0's cuda.h
typedef enum cudaError_enum {
    CUDA_SUCCESS                    = 0,        ///< No errors
    CUDA_ERROR_INVALID_VALUE        = 1,        ///< Invalid value
    CUDA_ERROR_OUT_OF_MEMORY        = 2,        ///< Out of memory
    CUDA_ERROR_NOT_INITIALIZED      = 3,        ///< Driver not initialized
    CUDA_ERROR_DEINITIALIZED        = 4,        ///< Driver deinitialized

    CUDA_ERROR_NO_DEVICE            = 100,      ///< No CUDA-capable device available
    CUDA_ERROR_INVALID_DEVICE       = 101,      ///< Invalid device

    CUDA_ERROR_INVALID_IMAGE        = 200,      ///< Invalid kernel image
    CUDA_ERROR_INVALID_CONTEXT      = 201,      ///< Invalid context
    CUDA_ERROR_CONTEXT_ALREADY_CURRENT = 202,   ///< Context already current
    CUDA_ERROR_MAP_FAILED           = 205,      ///< Map failed
    CUDA_ERROR_UNMAP_FAILED         = 206,      ///< Unmap failed
    CUDA_ERROR_ARRAY_IS_MAPPED      = 207,      ///< Array is mapped
    CUDA_ERROR_ALREADY_MAPPED       = 208,      ///< Already mapped
    CUDA_ERROR_NO_BINARY_FOR_GPU    = 209,      ///< No binary for GPU
    CUDA_ERROR_ALREADY_ACQUIRED     = 210,      ///< Already acquired
    CUDA_ERROR_NOT_MAPPED           = 211,      ///< Not mapped
    CUDA_ERROR_NOT_MAPPED_AS_ARRAY   = 212,      ///< Mapped resource not available for access as an array
    CUDA_ERROR_NOT_MAPPED_AS_POINTER = 213,      ///< Mapped resource not available for access as a pointer
    CUDA_ERROR_ECC_UNCORRECTABLE    = 214,      ///< Uncorrectable ECC error detected

    CUDA_ERROR_INVALID_SOURCE       = 300,      ///< Invalid source
    CUDA_ERROR_FILE_NOT_FOUND       = 301,      ///< File not found

    CUDA_ERROR_INVALID_HANDLE       = 400,      ///< Invalid handle

    CUDA_ERROR_NOT_FOUND            = 500,      ///< Not found

    CUDA_ERROR_NOT_READY            = 600,      ///< CUDA not ready

    CUDA_ERROR_LAUNCH_FAILED        = 700,      ///< Launch failed
    CUDA_ERROR_LAUNCH_OUT_OF_RESOURCES = 701,   ///< Launch exceeded resources
    CUDA_ERROR_LAUNCH_TIMEOUT       = 702,      ///< Launch exceeded timeout
    CUDA_ERROR_LAUNCH_INCOMPATIBLE_TEXTURING = 703, ///< Launch with incompatible texturing

    CUDA_ERROR_POINTER_IS_64BIT     = 800,      ///< Attempted to retrieve 64-bit pointer via 32-bit API function
    CUDA_ERROR_SIZE_IS_64BIT        = 801,      ///< Attempted to retrieve 64-bit size via 32-bit API function

    CUDA_ERROR_UNKNOWN              = 999       ///< Unknown error
} CUresult;

#ifdef _WIN32
#define CUDAAPI __stdcall
#else
#define CUDAAPI 
#endif

typedef struct CUdevice {
	int devno;
} CUdevice;

typedef struct opCUcontext {
	pthread_t tid;
} opCUcontext;

typedef opCUcontext *CUcontext;

typedef void *CUdeviceptr;

CUresult CUDAAPI cuInit(unsigned);
CUresult CUDAAPI cuDeviceGetCount(int *);
CUresult CUDAAPI cuDeviceGet(CUdevice *,int);
CUresult CUDAAPI cuCtxCreate(CUcontext *,unsigned,CUdevice);
CUresult CUDAAPI cuMemAlloc(CUdeviceptr *,unsigned);

#ifdef __cplusplus
}
#endif

#endif
