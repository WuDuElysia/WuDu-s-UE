cmake_minimum_required (VERSION 3.8)

add_library(WuDu_platform
	"private/Adlog.cpp"
	"private/AdWindow.cpp"
    "private/Window/AdGlfwWindow.cpp"
    "private/AdGraphicContext.cpp"
    "private/Graphic/AdVKGraphicContext.cpp" 
    "private/Graphic/AdVKDevice.cpp" 
    "private/Graphic/AdVKQueue.cpp" 
     "public/Graphic/AdVkSwapchain.h" "public/Graphic/AdVkSwapchain.cpp")

target_include_directories(WuDu_platform 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/External   # 添加 External 目录
        ${CMAKE_CURRENT_SOURCE_DIR}/public      # 添加 public 目录
)



option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_INSTALL OFF)

add_subdirectory(External/glfw)

target_link_libraries(WuDu_platform
    PRIVATE glfw
)

find_package(Vulkan REQUIRED)

if(Vulkan_FOUND)
    target_include_directories(WuDu_platform
        PUBLIC ${Vulkan_INCLUDE_DIRS}
    )
    target_link_libraries(WuDu_platform
        PRIVATE ${Vulkan_LIBRARY}
    )
    message("vulkan found")
endif()



