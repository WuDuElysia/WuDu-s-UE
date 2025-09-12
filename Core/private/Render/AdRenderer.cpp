#include "Render/AdRenderer.h"
#include "AdApplication.h"
#include "Graphic/AdVKQueue.h"
#include "Event/AdInputManager.h"
#include "Event/AdEvent.h"

namespace ade {
	/**
 * @brief AdRenderer构造函数
 * @details 初始化广告渲染器，创建用于Vulkan同步的信号量和栅栏
 * @param 无
 * @return 无返回值
 */
	AdRenderer::AdRenderer() {
		// 获取渲染上下文和设备句柄
		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		ade::AdVKDevice* device = renderCxt->GetDevice();

		// 调整同步对象容器大小以匹配缓冲区数量
		mImageAvailableSemaphores.resize(RENDERER_NUM_BUFFER);
		mSubmitedSemaphores.resize(RENDERER_NUM_BUFFER);
		mFrameFences.resize(RENDERER_NUM_BUFFER);

		// 配置信号量创建信息
		VkSemaphoreCreateInfo semaphoreInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};

		// 配置栅栏创建信息，初始状态为已触发
		VkFenceCreateInfo fenceInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT

		};

		// 为每个缓冲区创建同步对象（图像可用信号量、提交完成信号量和帧栅栏）
		for (int i = 0; i < RENDERER_NUM_BUFFER; i++) {
			CALL_VK(vkCreateSemaphore(device->GetHandle(), &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]));
			CALL_VK(vkCreateSemaphore(device->GetHandle(), &semaphoreInfo, nullptr, &mSubmitedSemaphores[i]));
			CALL_VK(vkCreateFence(device->GetHandle(), &fenceInfo, nullptr, &mFrameFences[i]));
		}

		// 订阅窗口大小变化事件
		ade::InputManager::GetInstance().Subscribe<ade::WindowResizeEvent>([this](ade::WindowResizeEvent& event) {
			// 窗口大小变化时，标记交换链需要重建
			mNeedSwapchainRecreate = true;
			LOG_T("Window resized to {0}x{1}", event.GetWidth(), event.GetHeight());
		});
	}

	AdRenderer::~AdRenderer() {
		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		ade::AdVKDevice* device = renderCxt->GetDevice();
		for (const auto& item : mImageAvailableSemaphores) {
			VK_D(Semaphore, device->GetHandle(), item);
		}
		for (const auto& item : mSubmitedSemaphores) {
			VK_D(Semaphore, device->GetHandle(), item);
		}
		for (const auto& item : mFrameFences) {
			VK_D(Fence, device->GetHandle(), item);
		}
	}

	/**
 * @brief 准备开始一帧渲染，主要工作包括等待上一帧完成、获取交换链图像索引，
 *        并在必要时重建交换链。
 *
 * 该函数会等待当前帧的 Fence 信号，确保前一帧已完成渲染，然后尝试从交换链中
 * 获取下一帧要渲染的图像索引。如果交换链失效（如窗口大小改变），则会尝试重建
 * 交换链，并根据是否尺寸发生变化决定是否需要更新渲染目标。
 *
 * @param[out] outImageIndex 返回获取到的交换链图像索引，用于后续渲染流程。
 *                           若传入 nullptr，则不输出该值。
 *
 * @return bool 如果交换链被重建且尺寸发生变化，返回 true，表示上层可能需要
 *              更新相关的渲染目标或视口设置；否则返回 false。
 */
	bool AdRenderer::Begin(int32_t* outImageIndex) {
		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		ade::AdVKDevice* device = renderCxt->GetDevice();
		ade::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		bool bShouldUpdateTarget = false;

		// 等待当前帧的 Fence，确保前一帧已完成
		CALL_VK(vkWaitForFences(device->GetHandle(), 1, &mFrameFences[mCurrentBuffer], VK_TRUE, UINT64_MAX));
		// 重置 Fence，为当前帧做准备
		CALL_VK(vkResetFences(device->GetHandle(), 1, &mFrameFences[mCurrentBuffer]));

		// 如果需要重建交换链（窗口大小变化触发），立即重建，不依赖AcquireImage的返回值
		if (mNeedSwapchainRecreate) {
			// 等待设备空闲以确保安全重建
			CALL_VK(vkDeviceWaitIdle(device->GetHandle()));
			VkExtent2D originExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			
			// 强制更新表面能力，确保获取最新的窗口大小
			LOG_T("Window size changed detected, recreating swapchain...");
			
			// 先手动更新表面能力，确保获取最新的窗口大小信息
			swapchain->ReCreate(); // 这会内部调用SetupSurfaceCapabilities()
			
			VkExtent2D newExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			
			// 检查尺寸是否真的发生变化
			if (originExtent.width != newExtent.width || originExtent.height != newExtent.height) {
				LOG_T("Swapchain size changed from {0}x{1} to {2}x{3}", 
					originExtent.width, originExtent.height, 
					newExtent.width, newExtent.height);
			}

			// 当交换链被重建时，总是返回true，确保渲染目标被更新
			bShouldUpdateTarget = true;
			LOG_T("Swapchain recreated due to resize, updating render targets");
			
			// 重置重建标志
			mNeedSwapchainRecreate = false;
		}

		// 创建一个 uint32_t 变量来接收图像索引
		uint32_t imageIndex;
		VkResult ret = swapchain->AcquireImage(imageIndex, mImageAvailableSemaphores[mCurrentBuffer]);

		// 如果交换链失效（如窗口大小改变），则尝试重建
		if (ret == VK_ERROR_OUT_OF_DATE_KHR) {
			// 等待设备空闲以确保安全重建
			CALL_VK(vkDeviceWaitIdle(device->GetHandle()));
			VkExtent2D originExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			
			// 重建交换链
			bool bSuc = swapchain->ReCreate();

			VkExtent2D newExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			// 当交换链被重建时，总是返回true，确保渲染目标被更新
			if (bSuc) {
				bShouldUpdateTarget = true;
				LOG_T("Swapchain recreated due to out of date, updating render targets");
			}

			// 重新获取图像索引
			ret = swapchain->AcquireImage(imageIndex, mImageAvailableSemaphores[mCurrentBuffer]);
			if (ret != VK_SUCCESS && ret != VK_SUBOPTIMAL_KHR) {
				LOG_E("Recreate swapchain error: {0}", vk_result_string(ret));
			}
		}

		// 将结果赋值给输出参数
		if (outImageIndex) {
			*outImageIndex = static_cast<int32_t>(imageIndex);
		}

		return bShouldUpdateTarget;
	}

	/**
 * @brief 结束渲染过程，提交命令缓冲区并呈现图像
 *
 * 该函数负责提交图形命令缓冲区到队列，执行图像呈现操作，
 * 并处理交换链重建等特殊情况。
 *
 * @param imageIndex 当前要呈现的交换链图像索引
 * @param cmdBuffers 要提交的命令缓冲区数组
 *
 * @return bool 是否需要更新渲染目标尺寸，当交换链重建且尺寸发生变化时返回true
 */
	bool AdRenderer::End(int32_t imageIndex, const std::vector<VkCommandBuffer>& cmdBuffers) {
		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		ade::AdVKDevice* device = renderCxt->GetDevice();
		ade::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();
		bool bShouldUpdateTarget = false;

		// 提交命令缓冲区到图形队列，并等待信号量
		device->GetFirstGraphicQueue()->Submit(cmdBuffers, { mImageAvailableSemaphores[mCurrentBuffer] }, { mSubmitedSemaphores[mCurrentBuffer] }, mFrameFences[mCurrentBuffer]);

		// 执行图像呈现操作
		VkResult ret = swapchain->Present(imageIndex, { mSubmitedSemaphores[mCurrentBuffer] });

		// 处理呈现结果为次优的情况，需要重建交换链
		if (ret == VK_SUBOPTIMAL_KHR) {
			CALL_VK(vkDeviceWaitIdle(device->GetHandle()));
			VkExtent2D originExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			bool bSuc = swapchain->ReCreate();

			VkExtent2D newExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			// 当交换链被重建时，总是返回true，确保渲染目标被更新
			if (bSuc) {
				bShouldUpdateTarget = true;
				LOG_T("Swapchain recreated due to suboptimal, updating render targets");
			}
		}

		// 等待设备空闲并更新当前缓冲区索引
		CALL_VK(vkDeviceWaitIdle(device->GetHandle()));
		mCurrentBuffer = (mCurrentBuffer + 1) % RENDERER_NUM_BUFFER;
		return bShouldUpdateTarget;
	}
}