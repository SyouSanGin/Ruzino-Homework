#include <memory>

#include "GCore/Components/MeshComponent.h"
#include "GCore/geom_payload.hpp"
#include "RHI/cuda.hpp"
#include "nodes/core/def/node_def.hpp"
#include "rzpython/rzpython.hpp"
#include "rzsim/reduced_order_basis.h"


using namespace Ruzino;

NODE_DEF_OPEN_SCOPE

// Storage for caching GPU buffers
struct BssDeducerStorage {
    cuda::CUDALinearBufferHandle vertices_buffer;
    bool initialized = false;
    int num_vertices = 0;

    constexpr static bool has_storage = false;
};

NODE_DECLARATION_FUNCTION(bss_deducer)
{
    b.add_input<Geometry>("Geometry");
    b.add_input<float>("Shape Code").default_val(0.0f).min(0.0f).max(1.0f);
    b.add_input<int>("Eigenfunction Index").default_val(0).min(0).max(3);
    b.add_input<std::string>("Result Name").default_val("eigenfunction");
    b.add_output<Geometry>("Geometry");
    b.add_output<std::shared_ptr<ReducedOrderedBasis>>("Reduced Basis");
}

NODE_EXECUTION_FUNCTION(bss_deducer)
{
    auto input_geom = params.get_input<Geometry>("Geometry");
    auto reduced_basis = std::make_shared<ReducedOrderedBasis>();
    auto& storage = params.get_storage<BssDeducerStorage&>();
    
    float shape_code = params.get_input<float>("Shape Code");
    int eigenfunction_idx = params.get_input<int>("Eigenfunction Index");
    std::string result_name = params.get_input<std::string>("Result Name");

    try {
        // Import torch and deducer module
        python::call<void>("import torch");
        python::call<void>(
            "import sys\n"
            "sys.path.insert(0, "
            "r'C:"
            "\\Users\\Pengfei\\WorkSpace\\Ruzino\\source\\Core\\rzsim\\geometry"
            "_nodes')\n"
            "import deducer");

        // Initialize basis set (will load model on first call)
        std::string init_result = python::call<std::string>("deducer.initialize_basis_set()");
        spdlog::info("Basis set initialization: {}", init_result);

        // Get vertices from geometry
        auto mesh_component = input_geom.get_component<MeshComponent>();
        if (mesh_component) {
            std::vector<glm::vec3> vertices = mesh_component->get_vertices();

            // Create or update GPU buffer for vertices
            if (!storage.initialized ||
                storage.num_vertices != vertices.size()) {
                storage.vertices_buffer =
                    cuda::create_cuda_linear_buffer(vertices);
                storage.num_vertices = vertices.size();
                storage.initialized = true;
            }
            else {
                // Update existing buffer
                storage.vertices_buffer->assign_host_vector(vertices);
            }

            spdlog::info(
                "Running inference on {} vertices with shape_code={:.2f}", 
                vertices.size(), shape_code);

            // Zero-copy send CUDA array to Python
            float* gpu_ptr = storage.vertices_buffer->get_device_ptr<float>();
            python::send(
                "vertices_cuda",
                python::CudaArrayView<float>{ gpu_ptr,
                                              { vertices.size(), 3 } });
            
            // Send shape code to Python
            python::send("shape_code_value", shape_code);

            // Inference all 4 eigenfunctions (0, 1, 2, 3)
            reduced_basis->basis.resize(4);
            std::vector<float> selected_results;
            
            for (int i = 0; i < 4; i++) {
                // Call Python inference function for each eigenfunction
                python::call<void>(
                    "import numpy as np\n"
                    "import torch\n"
                    "inference_result_" + std::to_string(i) + " = deducer.run_inference_on_vertices("
                    "vertices_cuda, eigenfunction_idx=" + std::to_string(i) + ", "
                    "shape_code=torch.tensor([shape_code_value]))");
                
                // Retrieve results as vector<float>
                std::vector<float> results = python::call<std::vector<float>>(
                    "inference_result_" + std::to_string(i) + ".tolist()");
                
                if (results.size() != vertices.size()) {
                    spdlog::error(
                        "Eigenfunction {} result size ({}) does not match vertex count ({})",
                        i, results.size(), vertices.size());
                    continue;
                }
                
                // Store in reduced basis
                reduced_basis->basis[i] = Eigen::VectorXf::Map(results.data(), results.size());
                
                // Keep the selected eigenfunction for quantity
                if (i == eigenfunction_idx) {
                    selected_results = results;
                }
                
                spdlog::info("Computed eigenfunction {}", i);
            }
            
            // Add only the selected eigenfunction as vertex scalar quantity
            if (!selected_results.empty()) {
                mesh_component->add_vertex_scalar_quantity(result_name, selected_results);
                spdlog::info(
                    "Added eigenfunction {} as '{}' vertex attribute", 
                    eigenfunction_idx, result_name);
            }
        }
        else {
            spdlog::warn("No mesh component found in geometry");
        }
    }
    catch (const std::exception& e) {
        spdlog::error("Python call failed: {}", e.what());
    }

    params.set_output<Geometry>("Geometry", std::move(input_geom));
    params.set_output<std::shared_ptr<ReducedOrderedBasis>>(
        "Reduced Basis", std::move(reduced_basis));

    return true;
}

NODE_DECLARATION_UI(bss_deducer);

NODE_DEF_CLOSE_SCOPE