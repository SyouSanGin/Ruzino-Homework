#include <stdio.h>
#include <thrust/device_vector.h>
#include <thrust/reduce.h>
#include <thrust/sort.h>
#include <thrust/transform.h>
#include <thrust/transform_reduce.h>
#include <thrust/unique.h>

#include <RHI/cuda.hpp>
#include <cstddef>
#include <map>
#include <set>

#include "RZSolver/Solver.hpp"
#include "rzsim_cuda/mass_spring_implicit.cuh"

RUZINO_NAMESPACE_OPEN_SCOPE

namespace rzsim_cuda {

// Kernel to extract edges from triangles
__global__ void
extract_edges_kernel(const int* triangles, int num_triangles, int* edge_pairs)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= num_triangles)
        return;

    int base_idx = tid * 3;
    int v0 = triangles[base_idx];
    int v1 = triangles[base_idx + 1];
    int v2 = triangles[base_idx + 2];

    // Each triangle produces 3 edges
    int output_base = tid * 6;

    // Edge 0-1
    edge_pairs[output_base + 0] = min(v0, v1);
    edge_pairs[output_base + 1] = max(v0, v1);

    // Edge 1-2
    edge_pairs[output_base + 2] = min(v1, v2);
    edge_pairs[output_base + 3] = max(v1, v2);

    // Edge 2-0
    edge_pairs[output_base + 4] = min(v2, v0);
    edge_pairs[output_base + 5] = max(v2, v0);
}

// Functor for comparing edge pairs
struct EdgePairEqual {
    __host__ __device__ bool operator()(
        const thrust::tuple<int, int>& a,
        const thrust::tuple<int, int>& b) const
    {
        return thrust::get<0>(a) == thrust::get<0>(b) &&
               thrust::get<1>(a) == thrust::get<1>(b);
    }
};

// Kernel to separate interleaved edges
__global__ void separate_edges_kernel(
    const int* interleaved,
    int* first,
    int* second,
    int num_edges)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= num_edges)
        return;

    first[tid] = interleaved[tid * 2];
    second[tid] = interleaved[tid * 2 + 1];
}

// Kernel to copy separated edges back to interleaved format
__global__ void interleave_edges_kernel(
    const int* edge_first,
    const int* edge_second,
    int* output,
    int num_edges)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= num_edges)
        return;

    output[tid * 2] = edge_first[tid];
    output[tid * 2 + 1] = edge_second[tid];
}

cuda::CUDALinearBufferHandle build_edge_set_gpu(
    cuda::CUDALinearBufferHandle positions,
    cuda::CUDALinearBufferHandle edges)
{
    // Get triangle count
    size_t num_triangles = edges->getDesc().element_count / 3;

    printf("[GPU] Building edge set from %zu triangles\n", num_triangles);

    // Allocate temporary buffer for all edges (3 edges per triangle)
    thrust::device_vector<int> all_edges(num_triangles * 6);

    // Launch kernel to extract edges
    int block_size = 256;
    int num_blocks = (num_triangles + block_size - 1) / block_size;

    extract_edges_kernel<<<num_blocks, block_size>>>(
        edges->get_device_ptr<int>(),
        num_triangles,
        thrust::raw_pointer_cast(all_edges.data()));

    cudaDeviceSynchronize();

    // Create vectors for edge pairs
    thrust::device_vector<int> edge_first(num_triangles * 3);
    thrust::device_vector<int> edge_second(num_triangles * 3);

    // Separate the interleaved edge data using kernel
    int sep_blocks = (num_triangles * 3 + block_size - 1) / block_size;
    separate_edges_kernel<<<sep_blocks, block_size>>>(
        thrust::raw_pointer_cast(all_edges.data()),
        thrust::raw_pointer_cast(edge_first.data()),
        thrust::raw_pointer_cast(edge_second.data()),
        num_triangles * 3);

    cudaDeviceSynchronize();

    // Create zip iterator
    auto edge_begin = thrust::make_zip_iterator(
        thrust::make_tuple(edge_first.begin(), edge_second.begin()));
    auto edge_end = thrust::make_zip_iterator(
        thrust::make_tuple(edge_first.end(), edge_second.end()));

    // Sort edges
    thrust::sort(
        edge_begin,
        edge_end,
        [] __device__(
            const thrust::tuple<int, int>& a,
            const thrust::tuple<int, int>& b) {
            if (thrust::get<0>(a) != thrust::get<0>(b))
                return thrust::get<0>(a) < thrust::get<0>(b);
            return thrust::get<1>(a) < thrust::get<1>(b);
        });

    // Remove duplicates
    auto new_end = thrust::unique(edge_begin, edge_end, EdgePairEqual());

    // Calculate unique edge count
    size_t num_unique_edges = new_end - edge_begin;

    printf("[GPU] Found %zu unique edges\n", num_unique_edges);

    // Copy unique edges to output buffer (interleaved format)
    auto output_buffer =
        cuda::create_cuda_linear_buffer<int>(num_unique_edges * 2);

    int* output_ptr = output_buffer->get_device_ptr<int>();

    // Use kernel to interleave the data
    int copy_blocks = (num_unique_edges + block_size - 1) / block_size;
    interleave_edges_kernel<<<copy_blocks, block_size>>>(
        thrust::raw_pointer_cast(edge_first.data()),
        thrust::raw_pointer_cast(edge_second.data()),
        output_ptr,
        num_unique_edges);

    cudaDeviceSynchronize();

    return output_buffer;
}
}  // namespace rzsim_cuda

RUZINO_NAMESPACE_CLOSE_SCOPE
