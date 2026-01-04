#include <gtest/gtest.h>
#include <rzsim/rzsim.h>
#include <GCore/GOP.h>
#include <GCore/Components/MeshComponent.h>

// Forward declarations for CUDA initialization
namespace Ruzino { namespace cuda {
    extern int cuda_init();
    extern int cuda_shutdown();
}}
#include <iostream>

using namespace Ruzino;

TEST(AdjacencyMap, SimpleTriangle)
{
    // Create a simple triangle mesh
    Geometry mesh = Geometry::CreateMesh();
    auto meshComp = mesh.get_component<MeshComponent>();

    // Triangle vertices: 0, 1, 2
    std::vector<glm::vec3> vertices = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    };

    std::vector<int> faceVertexCounts = {3};
    std::vector<int> faceVertexIndices = {0, 1, 2};

    meshComp->set_vertices(vertices);
    meshComp->set_face_vertex_counts(faceVertexCounts);
    meshComp->set_face_vertex_indices(faceVertexIndices);

    // Test GPU version
    auto adjacencyGPU = get_adjcency_map_gpu(mesh);
    ASSERT_NE(adjacencyGPU, nullptr);

    // Test CPU version (just downloads from GPU)
    auto adjacencyCPU = get_adjcency_map(mesh);
    ASSERT_GT(adjacencyCPU.size(), 0);

    std::cout << "Adjacency map for triangle:\n";
    for (size_t i = 0; i < adjacencyCPU.size(); i++) {
        std::cout << adjacencyCPU[i] << " ";
    }
    std::cout << std::endl;

    // Each vertex should be connected to the other two
    // Total unique edges: 3 edges * 2 directions = 6, but we remove duplicates
    // So we expect each vertex to have 2 neighbors
}

TEST(AdjacencyMap, Quad)
{
    // Create a quad mesh
    Geometry mesh = Geometry::CreateMesh();
    auto meshComp = mesh.get_component<MeshComponent>();

    // Quad vertices: 0--1
    //                |  |
    //                3--2
    std::vector<glm::vec3> vertices = {
        glm::vec3(0.0f, 1.0f, 0.0f),  // 0
        glm::vec3(1.0f, 1.0f, 0.0f),  // 1
        glm::vec3(1.0f, 0.0f, 0.0f),  // 2
        glm::vec3(0.0f, 0.0f, 0.0f)   // 3
    };

    std::vector<int> faceVertexCounts = {4};
    std::vector<int> faceVertexIndices = {0, 1, 2, 3};

    meshComp->set_vertices(vertices);
    meshComp->set_face_vertex_counts(faceVertexCounts);
    meshComp->set_face_vertex_indices(faceVertexIndices);

    auto adjacencyCPU = get_adjcency_map(mesh);
    ASSERT_GT(adjacencyCPU.size(), 0);

    std::cout << "Adjacency map for quad:\n";
    for (size_t i = 0; i < adjacencyCPU.size(); i++) {
        std::cout << adjacencyCPU[i] << " ";
    }
    std::cout << std::endl;

    // Each vertex in a quad should be connected to 2 neighbors (along the edges)
}

TEST(AdjacencyMap, TwoTriangles)
{
    // Create two triangles sharing an edge
    Geometry mesh = Geometry::CreateMesh();
    auto meshComp = mesh.get_component<MeshComponent>();

    // Vertices:  0
    //           /|\
    //          / | \
    //         1--2--3
    std::vector<glm::vec3> vertices = {
        glm::vec3(0.5f, 1.0f, 0.0f),  // 0
        glm::vec3(0.0f, 0.0f, 0.0f),  // 1
        glm::vec3(0.5f, 0.0f, 0.0f),  // 2
        glm::vec3(1.0f, 0.0f, 0.0f)   // 3
    };

    std::vector<int> faceVertexCounts = {3, 3};
    std::vector<int> faceVertexIndices = {0, 1, 2, 0, 2, 3};

    meshComp->set_vertices(vertices);
    meshComp->set_face_vertex_counts(faceVertexCounts);
    meshComp->set_face_vertex_indices(faceVertexIndices);

    auto adjacencyCPU = get_adjcency_map(mesh);
    ASSERT_GT(adjacencyCPU.size(), 0);

    std::cout << "Adjacency map for two triangles:\n";
    for (size_t i = 0; i < adjacencyCPU.size(); i++) {
        std::cout << adjacencyCPU[i] << " ";
    }
    std::cout << std::endl;

    // Vertex 0 connects to: 1, 2, 3
    // Vertex 1 connects to: 0, 2
    // Vertex 2 connects to: 0, 1, 3
    // Vertex 3 connects to: 0, 2
}

int main(int argc, char** argv)
{
    // Initialize CUDA - use forward declaration to avoid namespace conflicts
    Ruzino::cuda::cuda_init();

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

    // Cleanup
    Ruzino::cuda::cuda_shutdown();

    return result;
}
