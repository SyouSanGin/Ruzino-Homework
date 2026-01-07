#pragma once

#include <RHI/cuda.hpp>

#include "RHI/internal/cuda_extension.hpp"
#include "api.h"

RUZINO_NAMESPACE_OPEN_SCOPE

namespace rzsim_cuda {
RZSIM_CUDA_API
cuda::CUDALinearBufferHandle build_edge_set_gpu(
    cuda::CUDALinearBufferHandle positions,
    cuda::CUDALinearBufferHandle edges);

}  // namespace rzsim_cuda

RUZINO_NAMESPACE_CLOSE_SCOPE
