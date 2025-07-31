#include <RHI/cuda.hpp>
int main()
{
    using namespace USTC_CG;

    // Initialize CUDA
    USTC_CG::cuda::cuda_init();

    auto cuda_buffer = USTC_CG::cuda::create_cuda_linear_buffer(
        USTC_CG::cuda::CUDALinearBufferDesc{ .element_count =
                                                 1024u * 1024u,  // 1 MB buffer
                                             .element_size = 4u

        },
        nullptr  // No initial data
    );

    cuda_buffer = nullptr;

    USTC_CG::cuda::cuda_shutdown();

    return 0;
}
