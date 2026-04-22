// AdGuiSystem.cpp - GUI系统主协调器
#include "Gui/AdGuiSystem.h"
#include "Gui/AdBuiltinComponentRegistration.h"
#include "Render/AdMaterial.h"
#include "AdApplication.h"
#include "AdFileUtil.h"
#include "Graphic/AdVKRenderPass.h"
#include <Window/AdGlfwWindow.h>

namespace WuDu {
	void AdGuiSystem::OnInit() {
		// 按顺序初始化各个组件
		mManager.OnInit();
		mRenderer.OnInit();
		mEventHandler.OnInit();

		// 获取设备和上下文以初始化ImGui Vulkan后端
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		AdVKDevice* device = renderCxt->GetDevice();
		auto vkContext = dynamic_cast<WuDu::AdVKGraphicContext*>(renderCxt->GetGraphicContext());

		// 初始化ImGui Vulkan后端
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = vkContext->GetInstance();
		init_info.PhysicalDevice = vkContext->GetPhyDevice();
		init_info.Device = device->GetHandle();
		init_info.QueueFamily = vkContext->GetGraphicQueueFamilyInfo().queueFamilyIndex;
		init_info.Queue = device->GetGraphicQueue(vkContext->GetGraphicQueueFamilyInfo().queueFamilyIndex)->GetHandle();
		init_info.DescriptorPool = mRenderer.GetDescriptorPool()->GetHandle();
		init_info.MinImageCount = device->GetSettings().swapchainImageCount;
		init_info.ImageCount = device->GetSettings().swapchainImageCount;
		init_info.UseDynamicRendering = false;
		init_info.PipelineInfoMain.RenderPass = mRenderer.GetRenderPass()->GetHandle();
		init_info.PipelineInfoMain.Subpass = 0;
		init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.PipelineInfoForViewports.RenderPass = mRenderer.GetRenderPass()->GetHandle();
		init_info.PipelineInfoForViewports.Subpass = 0;
		init_info.PipelineInfoForViewports.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info);

		// 等待初始化完成
		vkDeviceWaitIdle(device->GetHandle());

		// 注册所有内置组件到反射系统
		RegisterBuiltinComponents();

		// 扫描资源目录（使用引擎定义的资源根路径）
		mResourceBrowserPanel.ScanResources(AD_RES_ROOT_DIR);
	}

	void AdGuiSystem::OnRender() {
		// 旧版渲染方式（独立acquire/present），保留向后兼容
		mRenderer.OnRender();

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	VkCommandBuffer AdGuiSystem::OnRenderGui(int32_t imageIndex) {
		// 新版：只录制GUI命令缓冲区，不做acquire/present
		VkCommandBuffer guiCmd = mRenderer.RecordGuiCommands(imageIndex);

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		return guiCmd;
	}

	void AdGuiSystem::OnDestroy() {
		// 获取设备，用于等待操作完成
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		AdVKDevice* device = renderCxt->GetDevice();

		// 首先调用ImGui_ImplVulkan_Shutdown()，释放ImGui使用的所有Vulkan资源
		ImGui_ImplVulkan_Shutdown();

		// 等待ImGui资源释放完成
		vkDeviceWaitIdle(device->GetHandle());

		// 然后按顺序清理各个组件
		mEventHandler.OnDestroy();
		mManager.OnDestroy();
		mRenderer.OnDestroy();
	}

	void AdGuiSystem::OnBeforeRender() {
		// 渲染前的准备工作
	}

	void AdGuiSystem::ProcessPendingTextureLoads() {
		mInspectorPanel.ProcessPendingTextureLoads(mEditorContext);
	}

	// 转发给AdGuiManager的方法
	void AdGuiSystem::BeginGui() {
		mManager.BeginGui();
	}

	void AdGuiSystem::EndGui() {
		mManager.EndGui();
	}

	void AdGuiSystem::AddGuiFunction(const std::function<void()>& func) {
		mManager.AddGuiFunction(func);
	}

	// 转发给AdGuiRenderer的方法
	void AdGuiSystem::RebuildResources() {
		mRenderer.RebuildResources();
	}

	// 转发给AdGuiEventHandler的方法
	bool AdGuiSystem::ProcessInput() {
		return mEventHandler.ProcessInput();
	}

	// 设置场景编辑器资源
	void AdGuiSystem::SetResources(
			AdScene* scene,
			AdEntity* activeCamera,
			AdMesh* cubeMesh,
			AdMaterial* defaultMaterial
		) {
		mSceneEditor.SetResources(scene, activeCamera, cubeMesh, defaultMaterial);
		mEditorContext.scene = scene;
		mEditorContext.cubeMesh = cubeMesh;
		mEditorContext.defaultMaterial = static_cast<AdPBRMaterial*>(defaultMaterial);
		mSceneEditor.SetEditorContext(&mEditorContext);
	}
	
	// 设置全屏 Dockspace 和主菜单栏
	void AdGuiSystem::SetupDockspace() {
		// 主菜单栏（必须在 DockSpace 之前）
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("View")) {
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		// 创建全屏 Dockspace，PassthruCentralNode 让中央区域透明不遮挡场景
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

		// 首次运行时设置初始停靠布局
		if (mFirstTimeDockLayout) {
			mFirstTimeDockLayout = false;

			// 使用 DockSpaceOverViewport 返回的 ID（不能用 GetID("DockSpace")，那是不同的 ID）
			ImGui::DockBuilderRemoveNode(dockspaceId);
			ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

			// 左侧: Scene Hierarchy (20%)
			ImGuiID left, center;
			ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.2f, &left, &center);

			// 右侧: Inspector (25% of remaining)
			ImGuiID right, middle;
			ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, &right, &middle);

			// 底部: Resource Browser (25% of remaining center)
			ImGuiID bottom, viewportCenter;
			ImGui::DockBuilderSplitNode(middle, ImGuiDir_Down, 0.25f, &bottom, &viewportCenter);

			ImGui::DockBuilderDockWindow("Scene Hierarchy", left);
			ImGui::DockBuilderDockWindow("Inspector", right);
			ImGui::DockBuilderDockWindow("Resource Browser", bottom);
			ImGui::DockBuilderDockWindow("Viewport", viewportCenter);

			ImGui::DockBuilderFinish(dockspaceId);
		}
	}

	// 添加场景编辑器UI
	void AdGuiSystem::AddSceneEditor() {
		AddGuiFunction([this]() {
			// 每帧验证选中实体的有效性
			mEditorContext.ValidateSelection();

			// 渲染编辑器面板（独立浮动窗口，不占用主窗口）
			mHierarchyPanel.OnImGui(mEditorContext);
			mInspectorPanel.OnImGui(mEditorContext);
			mResourceBrowserPanel.OnImGui();

			// 保留现有的场景编辑器视口渲染
			mSceneEditor.AddSceneEditor();
		});
	}
}