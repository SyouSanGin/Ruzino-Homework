# CUDA library for rzsim - isolated to avoid flag propagation issues

RUZINO_ADD_LIB(
    rzsim_cuda
    SHARED
    WITH_CUDA
    PUBLIC_LIBS RHI Eigen3::Eigen
)

target_compile_definitions(rzsim_cuda PRIVATE BUILD_RZSIM_CUDA_MODULE)
