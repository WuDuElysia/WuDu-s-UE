cmake_minimum_required (VERSION 3.8)

add_library(WuDu_core
	private/AdApplication.cpp
 "private/Render/AdRenderTarget.cpp" "public/AdApplicationContext.h" "public/AdEntryPoint.h" "public/Render/AdMaterial.h" "public/Render/AdMesh.h" "public/Render/AdRenderContext.h" "public/Render/AdRenderer.h" "public/Render/AdSampler.h" "public/Render/AdTexture.h" "private/Render/AdMaterial.cpp" "private/Render/AdMesh.cpp" "private/Render/AdRenderContext.cpp" "private/Render/AdSampler.cpp" "private/Render/AdTexture.cpp" "public/ECS/Component/Material/AdBaseMaterialComponent.h" "public/ECS/Component/Material/AdMaterialComponent.h" "public/ECS/Component/Material/AdUnlitMaterialComponent.h" "public/ECS/Component/AdLookAtCameraComponent.h" "public/ECS/Component/AdTransformComponent.h" "public/ECS/System/AdBaseMaterialSystem.h" "public/ECS/System/AdMaterialSystem.h" "public/ECS/System/AdUnlitMaterialSystem.h" "public/ECS/AdComponent.h" "public/ECS/AdEntity.h" "public/ECS/AdNode.h" "public/ECS/AdScene.h" "public/ECS/AdSystem.h" "public/ECS/AdUUID.h" "private/ECS/Component/AdLookAtCameraComponent.cpp" "private/ECS/System/AdBaseMaterialSystem.cpp" "private/ECS/System/AdMaterialSystem.cpp" "private/ECS/System/AdUnlitMaterialSystem.cpp" "private/ECS/AdEntity.cpp" "private/ECS/AdNode.cpp" "private/ECS/AdScene.cpp" "private/ECS/AdUUID.cpp")

target_include_directories(WuDu_core PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/public   
)

target_link_libraries(WuDu_core PUBLIC
	WuDu_platform
)