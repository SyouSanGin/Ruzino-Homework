#pragma once

#include <RHI/cuda.hpp>
#include <memory>
#include <vector>

#include "api.h"

RUZINO_NAMESPACE_OPEN_SCOPE

// Forward declarations
class Geometry;

// Add your API functions here

RZSIM_API cuda::CUDALinearBufferHandle get_adjcency_map_gpu(const Geometry& g);

RZSIM_API std::vector<unsigned> get_adjcency_map(const Geometry& g);

RUZINO_NAMESPACE_CLOSE_SCOPE
