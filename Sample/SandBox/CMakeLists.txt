cmake_minimum_required (VERSION 3.8)


set(SHADERS
	00_hello_triangle.frag
	00_hello_triangle.vert
)

SPIRV_SHADERS(
	SHADER_HEADS
	SPIRV_VERSION
	1.0
	SOURCES
        ${SHADERS}
)
add_executable(SandBox 
			${SHADER_HEADS}
			Main.cpp
)

add_definitions(-DAD_RES_SHADER_DIR=\"${AD_DEFINE_RES_ROOT_DIR}/Shader/\")

target_link_libraries(SandBox PRIVATE WuDU_core)
target_link_libraries(SandBox PRIVATE WuDu_platform)