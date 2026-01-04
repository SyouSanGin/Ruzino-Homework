#pragma once

#include <vector>

#include "RHI/internal/cuda_extension.hpp"
#include "api.h"

RUZINO_NAMESPACE_OPEN_SCOPE

namespace rzsim_cuda {

// Pure CUDA implementation that doesn't depend on geometry library
// Returns device pointer (void*) and size through out parameter
RZSIM_CUDA_API cuda::CUDALinearBufferHandle compute_adjacency_map_gpu(
    cuda::CUDALinearBufferHandle vertices,
    cuda::CUDALinearBufferHandle faceVertexCounts,
    cuda::CUDALinearBufferHandle faceVertexIndices);

}  // namespace rzsim_cuda

RUZINO_NAMESPACE_CLOSE_SCOPE
