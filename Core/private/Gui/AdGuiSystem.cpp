// AdGuiSystem.cpp - GUI系统主协调器
#include "Gui/AdGuiSystem.h"
#include "AdApplication.h"
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
	}

	void AdGuiSystem::OnRender() {
		// 渲染GUI
		mRenderer.OnRender();

		// 处理多视口
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
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
			AdUnlitMaterial* defaultMaterial
		) {
		mSceneEditor.SetResources(scene, activeCamera, cubeMesh, defaultMaterial);
	}
	
	// 添加场景编辑器UI
	void AdGuiSystem::AddSceneEditor() {
		AddGuiFunction([this]() {
			mSceneEditor.AddSceneEditor();
		});
	}
}