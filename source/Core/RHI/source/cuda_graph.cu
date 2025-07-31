#include "RHI/api.h"
#if USTC_CG_WITH_CUDA

#include <cuda_runtime.h>

USTC_CG_NAMESPACE_OPEN_SCOPE

// Simple CUDA kernels for testing
__global__ void vector_add(float* a, float* b, float* c, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        c[tid] = a[tid] + b[tid];
    }
}

__global__ void vector_scale(float* a, float scale, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        a[tid] *= scale;
    }
}

__global__ void vector_init(float* a, float value, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        a[tid] = value;
    }
}

USTC_CG_NAMESPACE_CLOSE_SCOPE

#endif  // USTC_CG_WITH_CUDA
