cmake_minimum_required(VERSION 3.8)


set(SHADERS
	01_hello_buffer.frag
	01_hello_buffer.vert
	03_unlit_material.frag
        03_unlit_material.vert
	glsl_shader.vert
        glsl_shader.frag
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
        Main.cpp
)

# Add the generated SPIR-V files as dependencies
add_custom_target(04_ECS_Entity_Shaders DEPENDS ${SHADER_HEADS})
add_dependencies(04_ECS_Entity 04_ECS_Entity_Shaders)

target_link_libraries(04_ECS_Entity PRIVATE WuDu_core WuDu_platform)