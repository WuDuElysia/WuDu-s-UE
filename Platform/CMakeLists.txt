cmake_minimum_required (VERSION 3.8)

add_library(WuDu_platform
	"private/Adlog.cpp"
		
	"public/AdEngine.h" )

target_include_directories(WuDu_platform
	PUBLIC External
)