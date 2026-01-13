// AdGuiRenderer.cpp
#include "Gui/AdGuiRenderer.h"
#include "AdApplication.h"
#include "Graphic/AdVKRenderPass.h"
#include <Window/AdGlfwWindow.h>

namespace WuDu {
	AdGuiRenderer::AdGuiRenderer() {
	}

	void AdGuiRenderer::OnInit() {
		// 获取设备和上下文
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		AdVKDevice* device = renderCxt->GetDevice();
		WuDu::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		// 创建描述符池
		std::vector<VkDescriptorPoolSize> poolSizes = {
		    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 }
		};
		mImGuiDescriptorPool = std::make_shared<AdVKDescriptorPool>(device, 100, poolSizes);

		// 创建GUI渲染通道
		CreateGUIRenderPass();
	}

	void AdGuiRenderer::OnRender() {
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		WuDu::AdVKDevice* device = renderCxt->GetDevice();
		WuDu::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();
		ImDrawData* draw_data = ImGui::GetDrawData();

		// 检查窗口大小是否变化，如果变化了就重建资源
		ImGuiIO& io = ImGui::GetIO();
		if (io.DisplaySize.x != static_cast<float>(swapchain->GetWidth()) ||
			io.DisplaySize.y != static_cast<float>(swapchain->GetHeight())) {
			RebuildResources();
			return; // 资源重建后，等待下一帧再渲染
		}

		// 获取当前的交换链图像索引
		int32_t imageIndex;
		bool swapchainRebuilt = mGUIRenderer->Begin(&imageIndex);

		// 如果交换链被重建，我们需要重建所有GUI资源
		if (swapchainRebuilt) {
			RebuildResources();
			return; // 资源重建后，等待下一帧再渲染
		}

		// 确保命令缓冲区数组足够大（安全检查）
		if (imageIndex < 0 || imageIndex >= static_cast<int32_t>(mGUICmdBuffers.size())) {
			// 索引无效，重建资源
			RebuildResources();
			return;
		}

		try {
			VkCommandBuffer cmdBuffer = mGUICmdBuffers[imageIndex];
			WuDu::AdVKCommandPool::BeginCommandBuffer(cmdBuffer);

			// 使用GUI专用的渲染目标进行渲染
			mGUIRenderTarget->Begin(cmdBuffer);

			// 渲染ImGui
			if (draw_data && draw_data->CmdListsCount > 0) {
				ImGui_ImplVulkan_RenderDrawData(draw_data, cmdBuffer);
			}

			mGUIRenderTarget->End(cmdBuffer);

			WuDu::AdVKCommandPool::EndCommandBuffer(cmdBuffer);

			// 提交GUI渲染命令
			if (mGUIRenderer->End(imageIndex, { cmdBuffer })) {
				// 如果在End过程中交换链被重建，记录下来但不立即处理
				// 下一帧的Begin()会检测到并处理
			}
		}
		catch (const std::exception& e) {
			// 捕获任何异常，避免程序崩溃
			RebuildResources(); // 发生异常时重建资源
			return;
		}
	}

	void AdGuiRenderer::OnDestroy() {
		// 清理Vulkan资源
		mImGuiDescriptorPool.reset();
		mGUIRenderer.reset();
		mGUIRenderTarget.reset();
		mGUIRenderPass.reset();

		// 清理命令缓冲区
		if (!mGUICmdBuffers.empty()) {
			mGUICmdBuffers.clear();
		}
	}

	void AdGuiRenderer::RebuildResources() {
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		auto device = renderCxt->GetDevice();
		auto swapchain = renderCxt->GetSwapchain();

		// 等待所有命令执行完成，确保安全释放资源
		vkDeviceWaitIdle(device->GetHandle());

		// 更新ImGui的DisplaySize以匹配新的窗口大小
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(static_cast<float>(swapchain->GetWidth()), static_cast<float>(swapchain->GetHeight()));

		// 销毁旧的GUI资源
		mGUICmdBuffers.clear();

		// 重新创建渲染通道、渲染目标和命令缓冲区
		// 先重置旧的资源
		mGUIRenderTarget.reset();
		mGUIRenderPass.reset();
		mGUIRenderer.reset(); // 也重置渲染器，确保与新的渲染目标兼容

		// 重新创建所有资源
		CreateGUIRenderPass();
	}

	void AdGuiRenderer::CreateGUIRenderPass() {
		// 获取设备和交换链
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		auto device = renderCxt->GetDevice();
		auto swapchain = renderCxt->GetSwapchain();

		// 定义GUI专用的渲染附件 - 只使用颜色附件，单采样
		std::vector<WuDu::Attachment> attachments = {
		    {
			.format = swapchain->GetSurfaceInfo().surfaceFormat.format,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,        // 保留3D场景内容
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // 从主渲染通道的最终布局开始
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // 最终呈现布局
			.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
		    }
		};

		// 定义GUI子通道 - 只使用颜色附件
		std::vector<WuDu::RenderSubPass> subpasses = {
		    {
			.colorAttachments = { 0 },                  // 使用第一个颜色附件
			.sampleCount = VK_SAMPLE_COUNT_1_BIT        // 与附件采样数一致
		    }
		};

		// 创建GUI专用的渲染通道
		mGUIRenderPass = std::make_shared<WuDu::AdVKRenderPass>(device, attachments, subpasses);

		// 使用GUI专用渲染通道创建渲染目标
		mGUIRenderTarget = std::make_shared<WuDu::AdRenderTarget>(mGUIRenderPass.get());

		// 创建渲染器
		mGUIRenderer = std::make_shared<WuDu::AdRenderer>();

		// 设置清除值 - 使用透明清除以便在3D场景上叠加GUI
		mGUIRenderTarget->SetColorClearValue({ 0.0f, 0.0f, 0.0f, 0.0f });

		// 分配命令缓冲区
		mGUICmdBuffers = device->GetDefaultCmdPool()->AllocateCommandBuffer(swapchain->GetImages().size());
	}
}