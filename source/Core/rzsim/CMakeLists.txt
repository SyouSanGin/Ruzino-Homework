# Create the isolated CUDA library first
add_subdirectory(rzsim_cuda)

# Main rzsim library - links to both CUDA and geometry libraries
RUZINO_ADD_LIB(
	rzsim
	SHARED
	WITH_CUDA
	PUBLIC_LIBS RHI geometry rzsim_cuda
	SKIP_DIRS nodes renderer_nodes geometry_nodes rzsim_cuda
)

add_subdirectory(nodes)
add_subdirectory(renderer_nodes)
add_subdirectory(geometry_nodes)
