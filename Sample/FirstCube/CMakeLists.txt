cmake_minimum_required (VERSION 3.8)

set(SHADERS
        00_hello_triangle.vert
        00_hello_triangle.frag
        02_descriptor_set.frag
        02_descriptor_set.vert
        03_unlit_material.frag
        03_unlit_material.vert
        glsl_shader.vert
        glsl_shader.frag
)
spirv_shaders(
        SHADER_HEADERS
        SPIRV_VERSION
        1.0 # Currently targeting Vulkan 1.0
        SOURCES
        ${SHADERS}
)
add_definitions(-DAD_RES_SHADER_DIR=\"${AD_DEFINE_RES_ROOT_DIR}/Shader/\")
add_executable(FirstCube
			${SHADER_HEADS}
			Main.cpp
)



target_link_libraries(FirstCube PRIVATE WuDu_core)
target_link_libraries(FirstCube PRIVATE WuDu_platform)