#include <gtest/gtest.h>
#include <pxr/base/vt/array.h>

#include <fem_bem/fem_bem.hpp>

#include "GCore/Components/MeshComponent.h"
#include "GCore/algorithms/delauney.h"
#include "GCore/create_geom.h"

using namespace USTC_CG::fem_bem;
using namespace USTC_CG;

TEST(FEMBEMProblem, Laplacian)
{
    Geometry circle = create_circle_face(64, 1);

    auto delauneyed = geom_algorithm::delaunay(circle);

    ElementSolverDesc desc;
    desc.set_problem_dim(2)
        .set_element_family(ElementFamily::P_minus)
        .set_k(0)
        .set_equation_type(EquationType::Laplacian);

    auto solver = create_element_solver(desc);

    // Should be some notation from the periodic finite element table.
    solver->set_geometry(delauneyed);

    auto n = solver->get_boundary_count();

    EXPECT_EQ(n, 1);

    solver->set_boundary_condition(
        "sin(atan2(y,x)*2)", BoundaryCondition::Dirichlet, 0);

    std::vector<float> solution = solver->solve();

    auto mesh_component = delauneyed.get_component<MeshComponent>();

    mesh_component->add_vertex_scalar_quantity("solution", solution);
}

TEST(FEMBEMProblem, Laplacian3D)
{
    // Create a simple 3D geometry (sphere or tetrahedron)
    // For demonstration, we'll create a simple tetrahedron
    auto geometry = std::make_shared<Geometry>();
    auto mesh_comp = std::make_shared<MeshComponent>(geometry.get());
    geometry->attach_component(mesh_comp);

    // Define vertices of a unit tetrahedron
    std::vector<glm::vec3> vertices = {
        { 0.0f, 0.0f, 0.0f },     // vertex 0
        { 1.0f, 0.0f, 0.0f },     // vertex 1
        { 0.5f, 0.866f, 0.0f },   // vertex 2
        { 0.5f, 0.289f, 0.816f }  // vertex 3
    };

    // Define tetrahedral connectivity (4 triangular faces)
    std::vector<int> face_vertex_indices = {
        0, 1, 2,  // face 0: bottom
        0, 1, 3,  // face 1
        1, 2, 3,  // face 2
        0, 2, 3   // face 3
    };

    std::vector<int> face_vertex_counts = { 3, 3, 3, 3 };

    mesh_comp->set_vertices(vertices);
    mesh_comp->set_face_vertex_indices(face_vertex_indices);
    mesh_comp->set_face_vertex_counts(face_vertex_counts);

    ElementSolverDesc desc;
    desc.set_problem_dim(3)
        .set_element_family(ElementFamily::P_minus)
        .set_k(0)
        .set_equation_type(EquationType::Laplacian);

    auto solver = create_element_solver(desc);

    solver->set_geometry(*geometry);

    auto n = solver->get_boundary_count();
    EXPECT_EQ(n, 1);

    // Set boundary condition: f(x,y,z) = x^2 + y^2 + z^2
    solver->set_boundary_condition(
        "x*x + y*y + z*z", BoundaryCondition::Dirichlet, 0);

    std::vector<float> solution = solver->solve();

    mesh_comp->add_vertex_scalar_quantity("solution", solution);

    // Verify solution has correct size
    EXPECT_EQ(solution.size(), vertices.size());
}
