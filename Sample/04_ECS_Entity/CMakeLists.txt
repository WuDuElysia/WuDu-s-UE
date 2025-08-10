cmake_minimum_required(VERSION 3.8)

set(SHADERS
	
)

SPIRV_SHADERS(
	SHADER_HEADS
	SPIRV_VERSION
	1.0
	SOURCES
        ${SHADERS}
)
add_definitions(-DAD_RES_SHADER_DIR=\"${AD_DEFINE_RES_ROOT_DIR}/Shader/\")
add_executable(04_ECS_Entity
        ${SHADER_HEADERS}
        Main.cpp
)
target_link_libraries(04_ECS_Entity PRIVATE WuDu_core WuDu_platform)