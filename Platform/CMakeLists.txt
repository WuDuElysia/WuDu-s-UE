cmake_minimum_required (VERSION 3.8)

add_library(WuDu_platform
	                        "private/Adlog.cpp"
	                        "private/AdWindow.cpp"
                                "private/Window/AdGlfwWindow.cpp"
                                "private/AdGraphicContext.cpp"
                                "private/Graphic/AdVKGraphicContext.cpp" 
                                "private/Graphic/AdVKDevice.cpp" 
                                "private/Graphic/AdVKQueue.cpp" 
                                "private/Graphic/AdVkSwapchain.cpp"
                                "private/Graphic/AdVKRenderPass.cpp"
                                "private/Graphic/AdVKFrameBuffer.cpp"
                                "private/Graphic/AdVKImage.cpp" 
                                "private/Graphic/AdVKImageView.cpp" 
                                "private/Graphic/AdVKPipeline.cpp"
                                "private/Graphic/AdVKDescriptorSet.cpp" 
                                "private/Graphic/AdVKBuffer.cpp"
                                "private/Graphic/AdVKCommandBuffer.cpp"
                                "private/AdGeometryUtil.cpp" 
                                )

target_include_directories(WuDu_platform 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/External   # 添加 External 目录
        ${CMAKE_CURRENT_SOURCE_DIR}/public      # 添加 public 目录
)

target_sources(WuDu_platform
    PRIVATE
        External/imgui/imgui.cpp
        External/imgui/imgui_draw.cpp
        External/imgui/imgui_tables.cpp
        External/imgui/imgui_widgets.cpp
        External/imgui/imgui_demo.cpp # 可选，演示窗口
        External/imgui/backends/imgui_impl_glfw.cpp
        External/imgui/backends/imgui_impl_vulkan.cpp
        External/imgui/backends/imgui_impl_win32.h
)

option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_INSTALL OFF)

add_subdirectory(External/glfw)
add_subdirectory(External/glm)

target_link_libraries(WuDu_platform
    PUBLIC glfw
    PUBLIC glm

)

# 确保使用正确的 Vulkan 路径
if(DEFINED ENV{VULKAN_SDK})
    message("VULKAN_SDK environment variable found: $ENV{VULKAN_SDK}")
    set(Vulkan_INCLUDE_DIRS "$ENV{VULKAN_SDK}/Include")
    set(Vulkan_LIBRARY "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")
else()
    find_package(Vulkan REQUIRED)
endif()

if(DEFINED Vulkan_INCLUDE_DIRS AND DEFINED Vulkan_LIBRARY)
    target_include_directories(WuDu_platform
        PUBLIC ${Vulkan_INCLUDE_DIRS}
    )
    target_link_libraries(WuDu_platform
        PRIVATE ${Vulkan_LIBRARY}
    )
    message("Vulkan linked with: ${Vulkan_LIBRARY}")
else()
    message(FATAL_ERROR "Vulkan not found. Please check VULKAN_SDK environment variable.")
endif()



