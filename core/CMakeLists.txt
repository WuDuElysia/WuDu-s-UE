cmake_minimum_required (VERSION 3.8)

add_library(WuDU_core
	private/AdApplication.cpp
)

target_include_directories(WuDU_core PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/public   
)