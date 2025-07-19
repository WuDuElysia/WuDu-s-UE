# CMakeList.txt: WuDu's UE 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project(WuDu's_UE)

add_compile_options("/utf-8")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_definitions(-DAD_DEFINE_RES_ROOT_DIR=\"${CMAKE_SOURCE_DIR}/Resource/\")

if(WIN32)
	message("Platform: WIndows")
	add_definitions(-DAD_ENGINE_PLATFORM_WIN32)
elseif(UNIX)
	if(APPLE)
		message("Platform: MacOS")
		add_definitions(-DAD_ENGINE_PLATFORM_MACOS)
		else()
		message("Platform: Linux")
		add_definitions(-DAD_ENGINE_PLATFORM_LINUX)
	endif()
else()
	message("Unsupported platform!")
endif()

#vulkan
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
	message("Vulkan found: ${Vulkan_INCLUDE_DIRS}")
	include_directories(${Vulkan_INCLUDE_DIRS})
endif()

add_definitions(-DAD_ENGINE_GRAPHIC_API_VULKAN)
add_subdirectory(Editor)
add_subdirectory(Platform)
add_subdirectory(Core)
add_subdirectory(Sample)

