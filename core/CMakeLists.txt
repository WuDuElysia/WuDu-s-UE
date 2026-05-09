cmake_minimum_required (VERSION 3.8)

add_library(WuDu_core
	"private/AdApplication.cpp"
	 "private/Render/AdRenderTarget.cpp"
	  "private/Render/AdMaterial.cpp"
	  "private/Render/AdMesh.cpp"
	  "private/Render/AdRenderContext.cpp" 
	  "private/Render/AdRenderer.cpp"
	  "private/Render/AdSampler.cpp" 
	  "private/Render/AdTexture.cpp"
	 "private/ECS/Component/AdLookAtCameraComponent.cpp"
	 "private/ECS/System/AdBaseMaterialSystem.cpp"
	 "private/ECS/System/AdMaterialSystem.cpp"
	 "private/ECS/System/AdUnlitMaterialSystem.cpp"
	 "private/ECS/AdEntity.cpp"
	 "private/ECS/AdNode.cpp"
	 "private/ECS/AdScene.cpp"
	 "private/ECS/AdUUID.cpp" 
	 "private/ECS/Component/AdFirstPersonCameraComponent.cpp"
	 "private/Gui/AdGuiSystem.cpp"
	 "private/Gui/AdSceneEditor.cpp"
	 "private/Gui/AdGuiRenderer.cpp"
	 "private/Gui/AdGuiManager.cpp"
	 "private/Gui/AdGuiEventHandler.cpp"
	 "private/Gui/AdComponentRegistry.cpp"
	 "private/Gui/AdEditorContext.cpp"
	 "private/Gui/AdHierarchyPanel.cpp"
	 "private/Gui/AdInspectorPanel.cpp"
	 "private/Gui/AdBuiltinComponentRegistration.cpp"
	 "private/Gui/AdResourceBrowserPanel.cpp"
	 "private/Resource/AdModelLoader.cpp" 
	 "private/Resource/AdModelResource.cpp"
	 "private/Resource/AdResourceManager.cpp"
	 "private/ECS/System/AdPBRForwardMaterialSystem.cpp"
	 "private/ECS/System/AdPBRDeferredMaterialSystem.cpp"
	 "private/ECS/System/AdGridMaterialSystem.cpp"
	 "private/ECS/System/AdSkyboxMaterialSystem.cpp"
	 "private/Serialization/AdComponentSerializerRegistry.cpp"
	 "private/Serialization/AdBuiltinComponentSerializers.cpp"
	 "private/Serialization/AdSceneSerializer.cpp"
	 )

target_include_directories(WuDu_core PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/public   
)

target_link_libraries(WuDu_core PUBLIC
	WuDu_platform
)