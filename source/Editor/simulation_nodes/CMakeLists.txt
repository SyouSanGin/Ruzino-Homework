add_nodes(
	TARGET_NAME simulation_nodes 
	SRC_DIRS ${CMAKE_CURRENT_SOURCE_DIR} 
	DEP_LIBS stage nodes_system usd geometry usdShade Eigen3::Eigen autodiff igl::core igl_restricted::triangle
	COMPILE_DEFS NOMINMAX 
)