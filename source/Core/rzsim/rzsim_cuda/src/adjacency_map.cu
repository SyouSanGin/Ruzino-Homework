#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <thrust/reduce.h>
#include <thrust/scan.h>
#include <thrust/sort.h>
#include <thrust/unique.h>

#include <Eigen/Eigen>
#include <RHI/cuda.hpp>
#include <RHI/rhi.hpp>
#include <cub/cub.cuh>

#include "RHI/internal/cuda_extension.hpp"
#include "rzsim_cuda/adjacency_map.cuh"

RUZINO_NAMESPACE_OPEN_SCOPE

cuda::CUDALinearBufferHandle compute_adjacency_map_gpu(
    cuda::CUDALinearBufferHandle vertices,
    cuda::CUDALinearBufferHandle faceVertexCounts,
    cuda::CUDALinearBufferHandle faceVertexIndices)
{
    return cuda::create_cuda_linear_buffer(1);  // Placeholder
}

RUZINO_NAMESPACE_CLOSE_SCOPE
