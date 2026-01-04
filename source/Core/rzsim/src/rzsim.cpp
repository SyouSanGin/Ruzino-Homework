#include "rzsim/rzsim.h"

#include "GCore/Components/MeshComponent.h"
#include "GCore/GOP.h"
#include "rzsim_cuda/adjacency_map.cuh"

RUZINO_NAMESPACE_OPEN_SCOPE

cuda::CUDALinearBufferHandle get_adjcency_map_gpu(const Geometry& g)
{
    auto mesh = g.get_component<MeshComponent>();

    auto vertices = mesh->get_vertices();
    auto faceVertexCounts = mesh->get_face_vertex_counts();
    auto faceVertexIndices = mesh->get_face_vertex_indices();

    // Convert geometry to cuda buffer

    auto vertex_buffer = cuda::create_cuda_linear_buffer(vertices);
    auto face_counts_buffer = cuda::create_cuda_linear_buffer(faceVertexCounts);
    auto face_indices_buffer =
        cuda::create_cuda_linear_buffer(faceVertexIndices);

    // Call the pure CUDA implementation
    return rzsim_cuda::compute_adjacency_map_gpu(
        vertex_buffer, face_counts_buffer, face_indices_buffer);
}

std::vector<unsigned> get_adjcency_map(const Geometry& g)
{
    auto buffer = get_adjcency_map_gpu(g);
    return buffer->get_host_vector<unsigned>();
}

RUZINO_NAMESPACE_CLOSE_SCOPE
