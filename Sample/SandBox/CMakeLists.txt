cmake_minimum_required (VERSION 3.8)

set(SHADERS
        00_hello_triangle.vert
        00_hello_triangle.frag
        01_hello_buffer.vert
        01_hello_buffer.frag
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


add_executable(SandBox
        ${SHADER_HEADERS}
        Main.cpp
        
)
target_link_libraries(SandBox PRIVATE WuDu_core)