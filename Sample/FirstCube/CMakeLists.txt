cmake_minimum_required (VERSION 3.8)



add_executable(FirstCube
			${SHADER_HEADS}
			Main.cpp
)



target_link_libraries(FirstCube PRIVATE WuDu_core)
target_link_libraries(FirstCube PRIVATE WuDu_platform)