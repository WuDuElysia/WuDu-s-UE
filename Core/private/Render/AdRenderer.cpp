#include "Render/AdRenderer.h"
#include "AdApplication.h"
#include "Graphic/AdVKQueue.h"
#include "Event/AdInputManager.h"
#include "Event/AdEvent.h"

namespace WuDu {
	/**
 * @brief AdRenderer构造函数
 * @details 初始化渲染器，创建Vulkan同步对象(信号量和围栏)
 * @param 无
 * @return 无返回值
 */
	AdRenderer::AdRenderer() {
		// 获取渲染上下文和逻辑设备
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		WuDu::AdVKDevice* device = renderCxt->GetDevice();

		// 创建同步对象数组，大小与帧缓冲数量匹配
		mImageAvailableSemaphores.resize(RENDERER_NUM_BUFFER);
		mSubmitedSemaphores.resize(RENDERER_NUM_BUFFER);
		mFrameFences.resize(RENDERER_NUM_BUFFER);

		// 信号量创建信息
		VkSemaphoreCreateInfo semaphoreInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};

		// 围栏创建信息，初始状态为已触发
		VkFenceCreateInfo fenceInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT

		};

		// 为每个帧缓冲创建同步对象：图像可用信号量、提交信号量和帧围栏
		for (int i = 0; i < RENDERER_NUM_BUFFER; i++) {
			CALL_VK(vkCreateSemaphore(device->GetHandle(), &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]));
			CALL_VK(vkCreateSemaphore(device->GetHandle(), &semaphoreInfo, nullptr, &mSubmitedSemaphores[i]));
			CALL_VK(vkCreateFence(device->GetHandle(), &fenceInfo, nullptr, &mFrameFences[i]));
		}

		// 监听窗口大小变化事件
		WuDu::InputManager::GetInstance().Subscribe<WuDu::WindowResizeEvent>([this](WuDu::WindowResizeEvent& event) {
			// 当窗口大小变化时，标记需要重建交换链
			mNeedSwapchainRecreate = true;
			LOG_T("Window resized to {0}x{1}", event.GetWidth(), event.GetHeight());
		});
	}

	AdRenderer::~AdRenderer() {
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		WuDu::AdVKDevice* device = renderCxt->GetDevice();
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
 * @brief 准备开始一帧的渲染，包括等待上一帧完成、获取下一个交换链图像索引
 *        并在需要时重建交换链
 *
 * 该方法等待当前帧的Fence信号，确保上一帧已经完成渲染。然后从交换链
 * 获取下一帧要渲染的图像索引。如果交换链失效(如：窗口大小变化)，则会触发重建
 * 过程，并检查渲染目标尺寸是否变化，确定是否需要更新渲染目标。
 *
 * @param[out] outImageIndex 返回获取到的交换链图像索引，供后续渲染使用。
 *                           如果为nullptr，则忽略返回值。
 *
 * @return bool 如果交换链重建或尺寸发生变化，返回true，表示需要更新
 *              外部的渲染目标接口，否则返回false。
 */
	bool AdRenderer::Begin(int32_t* outImageIndex) {
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		WuDu::AdVKDevice* device = renderCxt->GetDevice();
		WuDu::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		bool bShouldUpdateTarget = false;

		// 等待当前帧的Fence，确保上一帧完成
		CALL_VK(vkWaitForFences(device->GetHandle(), 1, &mFrameFences[mCurrentBuffer], VK_TRUE, UINT64_MAX));
		// 重置Fence，为当前帧做准备
		CALL_VK(vkResetFences(device->GetHandle(), 1, &mFrameFences[mCurrentBuffer]));

		// 如果需要重建交换链(窗口大小变化)，先重建交换链再继续AcquireImage的调用
		if (mNeedSwapchainRecreate) {
			// 等待设备空闲，确保完全重建
			CALL_VK(vkDeviceWaitIdle(device->GetHandle()));
			VkExtent2D originExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			
			// 强制重建交换链，确保获取最新的窗口大小
			LOG_T("Window size changed detected, recreating swapchain...");
			
			// 重建交换链，会在内部调用SetupSurfaceCapabilities()
			swapchain->ReCreate();
			
			VkExtent2D newExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			
			// 检查尺寸是否有实际变化
			if (originExtent.width != newExtent.width || originExtent.height != newExtent.height) {
				LOG_T("Swapchain size changed from {0}x{1} to {2}x{3}", 
					originExtent.width, originExtent.height, 
					newExtent.width, newExtent.height);
			}

			// 当交换链重建时，设置为true，确保渲染目标被更新
			bShouldUpdateTarget = true;
			LOG_T("Swapchain recreated due to resize, updating render targets");
			
			// 清除重建标志
			mNeedSwapchainRecreate = false;
		}

		// 创建一个uint32_t变量存储图像索引
		uint32_t imageIndex;
		VkResult ret = swapchain->AcquireImage(imageIndex, mImageAvailableSemaphores[mCurrentBuffer]);

		// 如果获取失败(如：窗口大小变化)，则重建交换链
		if (ret == VK_ERROR_OUT_OF_DATE_KHR) {
			// 等待设备空闲，确保完全重建
			CALL_VK(vkDeviceWaitIdle(device->GetHandle()));
			VkExtent2D originExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			
			// 重建交换链
			bool bSuc = swapchain->ReCreate();

			VkExtent2D newExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			// 当交换链重建时，设置为true，确保渲染目标被更新
			if (bSuc) {
				bShouldUpdateTarget = true;
				LOG_T("Swapchain recreated due to out of date, updating render targets");
			}

			// 再次获取图像索引
			ret = swapchain->AcquireImage(imageIndex, mImageAvailableSemaphores[mCurrentBuffer]);
			if (ret != VK_SUCCESS && ret != VK_SUBOPTIMAL_KHR) {
				LOG_E("Recreate swapchain error: {0}", vk_result_string(ret));
			}
		}

		// 将结果返回给调用者
		if (outImageIndex) {
			*outImageIndex = static_cast<int32_t>(imageIndex);
		}

		return bShouldUpdateTarget;
	}

	/**
 * @brief 结束渲染过程，提交命令缓冲区并呈现图像
 *
 * 该方法将图像渲染命令缓冲区提交到队列，执行图像呈现操作
 * 并处理可能的交换链重建情况
 *
 * @param imageIndex 当前要呈现的交换链图像索引
 * @param cmdBuffers 要提交的命令缓冲区列表
 *
 * @return bool 是否需要更新渲染目标尺寸，当交换链重建或尺寸发生变化时返回true
 */
	bool AdRenderer::End(int32_t imageIndex, const std::vector<VkCommandBuffer>& cmdBuffers) {
		WuDu::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		WuDu::AdVKDevice* device = renderCxt->GetDevice();
		WuDu::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();
		bool bShouldUpdateTarget = false;

		// 提交命令缓冲区到图形队列，并等待信号量
		device->GetFirstGraphicQueue()->Submit(cmdBuffers, { mImageAvailableSemaphores[mCurrentBuffer] }, { mSubmitedSemaphores[mCurrentBuffer] }, mFrameFences[mCurrentBuffer]);

		// 执行图像呈现操作
		VkResult ret = swapchain->Present(imageIndex, { mSubmitedSemaphores[mCurrentBuffer] });

		// 如果呈现结果为次优的，则需要重建交换链
		if (ret == VK_SUBOPTIMAL_KHR) {
			CALL_VK(vkDeviceWaitIdle(device->GetHandle()));
			VkExtent2D originExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			bool bSuc = swapchain->ReCreate();

			VkExtent2D newExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			// 当交换链重建时，设置为true，确保渲染目标被更新
			if (bSuc) {
				bShouldUpdateTarget = true;
				LOG_T("Swapchain recreated due to suboptimal, updating render targets");
			}
		}

		// 等待设备完成当前所有操作
		CALL_VK(vkDeviceWaitIdle(device->GetHandle()));
		mCurrentBuffer = (mCurrentBuffer + 1) % RENDERER_NUM_BUFFER;
		return bShouldUpdateTarget;
	}
}