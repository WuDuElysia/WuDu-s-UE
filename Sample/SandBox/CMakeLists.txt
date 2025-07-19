cmake_minimum_required (VERSION 3.8)

add_executable(SandBox Main.cpp)

target_link_libraries(SandBox PRIVATE WuDU_core)
target_link_libraries(SandBox PRIVATE WuDu_platform)