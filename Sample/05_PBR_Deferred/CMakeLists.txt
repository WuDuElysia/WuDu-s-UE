cmake_minimum_required(VERSION 3.8)

set(SHADERS
	PBR_Deferred_GBuffer.vert
	PBR_Deferred_GBuffer.frag
	PBR_Deferred_Lighting.vert
	PBR_Deferred_Lighting.frag
	PBR_Deferred_IBL.vert
	PBR_Deferred_IBL.frag
	PBR_Deferred_Merge.vert
	PBR_Deferred_Merge.frag
	PBR_Deferred_PostProcess.vert
	PBR_Deferred_PostProcess.frag
	PBR_BRDF_LUT.vert
	PBR_BRDF_LUT.frag
	PBR_Irradiance.vert
	PBR_Irradiance.frag
	PBR_Prefilter.vert
	PBR_Prefilter.frag
	Scene_Grid.vert
	Scene_Grid.frag
	Skybox_Aurora.vert
	Skybox_Aurora.frag
)

SPIRV_SHADERS(
	SHADER_HEADS
	SPIRV_VERSION
	1.0
	SOURCES
	${SHADERS}
)
add_definitions(-DAD_RES_SHADER_DIR=\"${AD_DEFINE_RES_ROOT_DIR}/Shader/\")
add_executable(05_PBR_Deferred
	Main.cpp
)

# Add the generated SPIR-V files as dependencies
add_custom_target(05_PBR_Deferred_Shaders DEPENDS ${SHADER_HEADS})
add_dependencies(05_PBR_Deferred 05_PBR_Deferred_Shaders)

target_link_libraries(05_PBR_Deferred PRIVATE WuDu_core WuDu_platform)
