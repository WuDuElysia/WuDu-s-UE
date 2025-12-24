cmake_minimum_required(VERSION 3.8)


set(SHADERS
	02_descriptor_set.frag
	02_descriptor_set.vert
)

SPIRV_SHADERS(
	SHADER_HEADS
	SPIRV_VERSION
	1.0
	SOURCES
	${SHADERS}
)

add_definitions(-DAD_RES_SHADER_DIR=\"${AD_DEFINE_RES_ROOT_DIR}/Shader/\")
add_executable(03_Texture
	Main.cpp
)
add_custom_target(03_Texture_Shaders DEPENDS ${SHADER_HEADS})
add_dependencies(03_Texture 03_Texture_Shaders)

target_link_libraries(03_Texture PRIVATE WuDu_core WuDu_platform)