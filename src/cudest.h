#ifndef CUDEST_CUDEST
#define CUDEST_CUDEST

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

// Taken from 3.0's cuda.h

#define CUDA_VERSION 3000 /* 3.0 */

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

typedef enum CUcomputemode_enum {
    CU_COMPUTEMODE_DEFAULT    = 0,     ///< Default compute mode (Multiple contexts allowed per device)
    CU_COMPUTEMODE_EXCLUSIVE  = 1,     ///< Compute-exclusive mode (Only one context can be present on this device at a time)
    CU_COMPUTEMODE_PROHIBITED = 2      ///< Compute-prohibited mode (No contexts can be created on this device at this time)
} CUcomputemode;

typedef enum CUdevice_attribute_enum {
    CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_BLOCK = 1,  ///< Maximum number of threads per block
    CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_X = 2,        ///< Maximum block dimension X
    CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_Y = 3,        ///< Maximum block dimension Y
    CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_Z = 4,        ///< Maximum block dimension Z
    CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_X = 5,         ///< Maximum grid dimension X
    CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_Y = 6,         ///< Maximum grid dimension Y
    CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_Z = 7,         ///< Maximum grid dimension Z
    CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK = 8,    ///< Maximum shared memory available per block in bytes
    CU_DEVICE_ATTRIBUTE_SHARED_MEMORY_PER_BLOCK = 8,    ///< Deprecated, use CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK
    CU_DEVICE_ATTRIBUTE_TOTAL_CONSTANT_MEMORY = 9,  ///< Memory available on device for __constant__ variables in a CUDA C kernel in bytes
    CU_DEVICE_ATTRIBUTE_WARP_SIZE = 10,             ///< Warp size in threads
    CU_DEVICE_ATTRIBUTE_MAX_PITCH = 11,             ///< Maximum pitch in bytes allowed by memory copies
    CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_BLOCK = 12,   ///< Maximum number of 32-bit registers available per block
    CU_DEVICE_ATTRIBUTE_REGISTERS_PER_BLOCK = 12,   ///< Deprecated, use CU_DEVICE_ATTRIBUTE_MAX_REGISTERS_PER_BLOCK
    CU_DEVICE_ATTRIBUTE_CLOCK_RATE = 13,            ///< Peak clock frequency in kilohertz
    CU_DEVICE_ATTRIBUTE_TEXTURE_ALIGNMENT = 14,     ///< Alignment requirement for textures

    CU_DEVICE_ATTRIBUTE_GPU_OVERLAP = 15,           ///< Device can possibly copy memory and execute a kernel concurrently
    CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT = 16,  ///< Number of multiprocessors on device
    CU_DEVICE_ATTRIBUTE_KERNEL_EXEC_TIMEOUT = 17,   ///< Specifies whether there is a run time limit on kernels
    CU_DEVICE_ATTRIBUTE_INTEGRATED = 18,            ///< Device is integrated with host memory
    CU_DEVICE_ATTRIBUTE_CAN_MAP_HOST_MEMORY = 19,   ///< Device can map host memory into CUDA address space
    CU_DEVICE_ATTRIBUTE_COMPUTE_MODE = 20,          ///< Compute mode (See ::CUcomputemode for details)
    CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE1D_WIDTH = 21, ///< Maximum 1D texture width
    CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_WIDTH = 22, ///< Maximum 2D texture width
    CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_HEIGHT = 23,///< Maximum 2D texture height
    CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE3D_WIDTH = 24, ///< Maximum 3D texture width
    CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE3D_HEIGHT = 25,///< Maximum 3D texture height
    CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE3D_DEPTH = 26, ///< Maximum 3D texture depth
    CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_ARRAY_WIDTH = 27, ///< Maximum texture array width
    CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_ARRAY_HEIGHT = 28,///< Maximum texture array height
    CU_DEVICE_ATTRIBUTE_MAXIMUM_TEXTURE2D_ARRAY_NUMSLICES = 29, ///< Maximum slices in a texture array
    CU_DEVICE_ATTRIBUTE_SURFACE_ALIGNMENT = 30, ///< Alignment requirement for surfaces
    CU_DEVICE_ATTRIBUTE_CONCURRENT_KERNELS = 31, ///< Device can possibly execute multiple kernels concurrently
    CU_DEVICE_ATTRIBUTE_ECC_ENABLED = 32 ///< Device has ECC support enabled
} CUdevice_attribute;
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

typedef enum CUctx_flags_enum {
    CU_CTX_SCHED_AUTO  = 0,     ///< Automatic scheduling
    CU_CTX_SCHED_SPIN  = 1,     ///< Set spin as default scheduling
    CU_CTX_SCHED_YIELD = 2,     ///< Set yield as default scheduling
    CU_CTX_SCHED_MASK  = 0x3,
    CU_CTX_BLOCKING_SYNC = 4,   ///< Use blocking synchronization
    CU_CTX_MAP_HOST = 8,        ///< Support mapped pinned allocations
    CU_CTX_LMEM_RESIZE_TO_MAX = 16, ///< Keep local memory allocation after launch
    CU_CTX_FLAGS_MASK  = 0x1f
} CUctx_flags;

typedef void *CUdeviceptr;

CUresult CUDAAPI cuInit(unsigned);

CUresult CUDAAPI cuDriverGetVersion(int *);

CUresult CUDAAPI cuDeviceGetCount(int *);
CUresult CUDAAPI cuDeviceGet(CUdevice *,int);
CUresult CUDAAPI cuDeviceGetName(char *,int,CUdevice);
CUresult CUDAAPI cuDeviceComputeCapability(int *,int *,CUdevice);
CUresult CUDAAPI cuDeviceTotalMem(unsigned *,CUdevice);
CUresult CUDAAPI cuDeviceGetAttribute(int *,CUdevice_attribute,CUdevice);
CUresult CUDAAPI cuCtxCreate(CUcontext *,unsigned,CUdevice);

CUresult CUDAAPI cuMemGetInfo(unsigned *,unsigned *);
CUresult CUDAAPI cuMemAlloc(CUdeviceptr *,unsigned);

CUresult  CUDAAPI cuCtxDetach(CUcontext);

// Taken from 3.0's CUDA Runtime
struct cudaDeviceProp {
	char   name[256];                 ///< ASCII string identifying device
	size_t totalGlobalMem;            ///< Global memory available on device in bytes
	size_t sharedMemPerBlock;         ///< Shared memory available per block in bytes
	int    regsPerBlock;              ///< 32-bit registers available per block
	int    warpSize;                  ///< Warp size in threads
	size_t memPitch;                  ///< Maximum pitch in bytes allowed by memory copies
	int    maxThreadsPerBlock;        ///< Maximum number of threads per block
	int    maxThreadsDim[3];          ///< Maximum size of each dimension of a block
	int    maxGridSize[3];            ///< Maximum size of each dimension of a grid
	int    clockRate;                 ///< Clock frequency in kilohertz
	size_t totalConstMem;             ///< Constant memory available on device in bytes
	int    major;                     ///< Major compute capability
	int    minor;                     ///< Minor compute capability
	size_t textureAlignment;          ///< Alignment requirement for textures
	int    deviceOverlap;             ///< Device can concurrently copy memory and execute a kernel
	int    multiProcessorCount;       ///< Number of multiprocessors on device
	int    kernelExecTimeoutEnabled;  ///< Specified whether there is a run time limit on kernels
	int    integrated;                ///< Device is integrated as opposed to discrete
	int    canMapHostMemory;          ///< Device can map host memory with cudaHostAlloc/cudaHostGetDevicePointer
	int    computeMode;               ///< Compute mode (See ::cudaComputeMode)
	int    maxTexture1D;              ///< Maximum 1D texture size
	int    maxTexture2D[2];           ///< Maximum 2D texture dimensions
	int    maxTexture3D[3];           ///< Maximum 3D texture dimensions
	int    maxTexture2DArray[3];      ///< Maximum 2D texture array dimensions
	int    concurrentKernels;         ///< Device can possibly execute multiple kernels concurrently
	int    __cudaReserved[26];
};

#ifdef __cplusplus
}
#endif

#endif
