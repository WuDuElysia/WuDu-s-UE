// AdGuiSystem.h - 简化版本
#pragma once
#define NOMINMAX
#include "AdEngine.h"
#include "ECS/System/AdBaseMaterialSystem.h"
#include "Graphic/AdVKDescriptorSet.h"
#include "Graphic/AdVKCommandBuffer.h"
#include "Render/AdRenderTarget.h"
#include "Render/AdRenderer.h"
#include "Graphic/AdVKPipeline.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "Event/AdInputManager.h"
#include "Event/AdEvent.h"
#include "Event/AdEventAdaper.h"
#include "AdApplication.h"
#include "ECS/System/AdUnlitMaterialSystem.h"
#include "ECS/System/AdCameraControllerManager.h"
#include "ECS/Component/AdLookAtCameraComponent.h"
#include "ECS/Component/AdFirstPersonCameraComponent.h"

namespace ade {
	class AdGuiSystem{
	public:
		AdGuiSystem() : mGuiVisible(true) {}
		virtual ~AdGuiSystem() = default;

		// 系统生命周期
		void OnInit();
		void OnRender();
		void OnDestroy();
		void OnBeforeRender();

		// GUI管理
		void BeginGui();
		void EndGui();
		void AddGuiFunction(const std::function<void()>& func);
		void CreateGUIRenderPass();
		void RebuildResources();
		bool ProcessInput();
		void SetupGuiControls();
		void HandleMouseClick(ade::MouseClickEvent& event);
		void HandleMouseRelease(ade::MouseReleaseEvent& event);
		void HandleMouseMove(ade::MouseMoveEvent& event);
		void HandleMouseScroll(ade::MouseScrollEvent& event);

		// 设置GUI系统所需的资源
		void SetResources(
			AdScene* scene,
			AdEntity* activeCamera,
			AdMesh* cubeMesh,
			AdUnlitMaterial* defaultMaterial
		);
		// 添加场景编辑相关方法
		void SetScene(AdScene* scene) { mScene = scene; }
		void SetActiveCamera(AdEntity* camera) { mActiveCamera = camera; }

		// 场景编辑功能
		void AddSceneEditor();
		void ShowSceneHierarchy();
		void ShowTransformEditor();
		void HandleSceneViewport();

		// 实体选择相关
		AdEntity* GetSelectedEntity() const { return mSelectedEntity; }
		void SelectEntity(AdEntity* entity);

		void SelectEntityAtMousePosition(float mouseX, float mouseY);
		void HandleTransformDrag(const ImVec2& currentMousePos);
		float RayIntersectsCube(const glm::vec3& rayOrigin,
							const glm::vec3& rayDirection,
							const AdTransformComponent& transform);

	private:
		bool mGuiVisible;
		std::vector<std::function<void()>> mGuiFunctions;
		std::shared_ptr<AdVKDescriptorPool> mImGuiDescriptorPool;
		std::shared_ptr<AdVKRenderPass> mGUIRenderPass;    // GUI专用渲染通道
		std::shared_ptr<AdRenderTarget> mGUIRenderTarget;   // GUI专用渲染目标
		std::shared_ptr<AdRenderer> mGUIRenderer;
		std::vector<VkCommandBuffer> mGUICmdBuffers;          // GUI专用命令缓冲区
		bool m_MouseDragging = false;
		glm::vec2 m_LastMousePos;
		float m_LastTime = 0.0f;


		// 场景和摄像机
		AdScene* mScene = nullptr;
		AdEntity* mActiveCamera = nullptr;

		// 基础资源（由应用程序提供）
		AdMesh* mCubeMesh = nullptr;
		AdUnlitMaterial* mDefaultMaterial = nullptr;

		// 选中的实体
		AdEntity* mSelectedEntity = nullptr;

		bool mIsDraggingInViewport = false;
		glm::vec2 mLastMousePos;

		// 编辑模式
		enum class TransformMode { Translate, Rotate, Scale };
		TransformMode mCurrentTransformMode = TransformMode::Translate;

		std::shared_ptr<ade::AdTexture> mRenderTexture;
		std::shared_ptr<ade::AdSampler> mRenderTextureSampler;
		glm::uvec2 mViewportSize = { 800, 600 };

	};
}