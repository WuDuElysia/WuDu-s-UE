#include "Graphic/AdVkSwapchain.h"
#include "Graphic/AdVKGraphicContext.h"
#include "Graphic/AdVKDevice.h"
#include "Graphic/AdVKQueue.h"

namespace ade {
	AdVKSwapchain::AdVKSwapchain(AdVKGraphicContext* context, AdVKDevice* device) : mContext(context), mDevice(device) {
		ReCreate();
	}

	AdVKSwapchain::~AdVKSwapchain() {
		VK_D(SwapchainKHR, mDevice->GetHandle(), mHandle);
	}

	// 重新创建交换链
	bool AdVKSwapchain::ReCreate() {
		// 分隔日志输出，便于阅读
		LOG_D("-----------------------------");
		// 配置表面能力
		SetupSurfaceCapabilities();
		// 日志输出当前扩展、表面格式和呈现模式
		LOG_D("currentExtent : {0} x {1}", mSurfaceInfo.capabilities.currentExtent.width, mSurfaceInfo.capabilities.currentExtent.height);
		LOG_D("surfaceFormat : {0}", vk_format_string(mSurfaceInfo.surfaceFormat.format));
		LOG_D("presentMode   : {0}", vk_present_mode_string(mSurfaceInfo.presentMode));
		LOG_D("-----------------------------");

		// 初始化图像数量
		uint32_t imageCount = mDevice->GetSettings().swapchainImageCount;
		// 确保图像数量在表面能力的最小和最大图像数量之间
		if (imageCount < mSurfaceInfo.capabilities.minImageCount && mSurfaceInfo.capabilities.minImageCount > 0) {
			imageCount = mSurfaceInfo.capabilities.minImageCount;
		}
		if (imageCount > mSurfaceInfo.capabilities.maxImageCount && mSurfaceInfo.capabilities.maxImageCount > 0) {
			imageCount = mSurfaceInfo.capabilities.maxImageCount;
		}

		// 确定图像共享模式和队列家庭索引
		/*这段代码用于设置 Vulkan 图像的共享模式及相关的队列家族索引，具体功能如下：
		如果图形和呈现队列家族相同，则使用独占模式（VK_SHARING_MODE_EXCLUSIVE），无需指定队列家族索引。
		否则使用并发模式（VK_SHARING_MODE_CONCURRENT），并指定两个不同的队列家族索引，分别用于图形和呈现操作。*/
		VkSharingMode imageSharingMode;
		uint32_t queueFamilyIndexCount;
		uint32_t pQueueFamilyIndices[2] = { 0, 0 };
		if (mContext->IsSameGraphicPresentQueueFamily()) {
			imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			queueFamilyIndexCount = 0;
		}
		else {
			imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			queueFamilyIndexCount = 2;
			pQueueFamilyIndices[0] = mContext->GetGraphicQueueFamilyInfo().queueFamilyIndex;
			pQueueFamilyIndices[1] = mContext->GetPresentQueueFamilyInfo().queueFamilyIndex;
		}

		// 保存旧的交换链句柄
		VkSwapchainKHR oldSwapchain = mHandle;

		// 填充交换链创建信息结构体
		VkSwapchainCreateInfoKHR swapchainInfo = {
			// 指定结构体类型
			swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			// 指向下一个结构体的指针，此处不需要链接其他结构体
			swapchainInfo.pNext = nullptr,
			// 保留字段，用于未来使用，目前应设置为0
			swapchainInfo.flags = 0,
			// 分配给交换链的表面
			swapchainInfo.surface = mContext->GetSurface(),
			// 最小图像数量，影响渲染性能和内存使用
			swapchainInfo.minImageCount = imageCount,
			// 图像的格式
			swapchainInfo.imageFormat = mSurfaceInfo.surfaceFormat.format,
			// 图像的颜色空间
			swapchainInfo.imageColorSpace = mSurfaceInfo.surfaceFormat.colorSpace,
			// 图像的分辨率
			swapchainInfo.imageExtent = mSurfaceInfo.capabilities.currentExtent,
			// 图像的层数，通常为1，用于2D渲染
			swapchainInfo.imageArrayLayers = 1,
			// 图像的使用方式，此处作为颜色附件
			swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			// 图像共享模式，决定如何在队列之间共享图像
			swapchainInfo.imageSharingMode = imageSharingMode,
			// 队列族索引数量，用于支持图像共享模式
			swapchainInfo.queueFamilyIndexCount = queueFamilyIndexCount,
			// 队列族索引指针，指定参与图像共享的队列族
			swapchainInfo.pQueueFamilyIndices = pQueueFamilyIndices,
			// 表面变换方式，此处设置为无变换
			swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			// 组合alpha值，此处设置为继承
			swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			//swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
			// 呈现模式，影响渲染的流畅性和延迟
			swapchainInfo.presentMode = mSurfaceInfo.presentMode,
			// 是否允许图像被裁剪，减少需要呈现的像素数量
			swapchainInfo.clipped = VK_FALSE,
			// 旧的交换链，用于交换链的重建
			swapchainInfo.oldSwapchain = oldSwapchain
		};
		// 创建交换链
		VkResult ret = vkCreateSwapchainKHR(mDevice->GetHandle(), &swapchainInfo, nullptr, &mHandle);
		if (ret != VK_SUCCESS) {
			LOG_E("{0} : {1}", __FUNCTION__, vk_result_string(ret));
			return false;
		}
		// 日志输出交换链创建信息
		LOG_T("Swapchain {0} : old: {1}, new: {2}, image count: {3}, format: {4}, present mode : {5}", __FUNCTION__, (void*)oldSwapchain, (void*)mHandle, imageCount,
			vk_format_string(mSurfaceInfo.surfaceFormat.format), vk_present_mode_string(mSurfaceInfo.presentMode));

		// 获取交换链图像数量并调整图像数组大小
		uint32_t swapchainImageCount;
		ret = vkGetSwapchainImagesKHR(mDevice->GetHandle(), mHandle, &swapchainImageCount, nullptr);
		mImages.resize(swapchainImageCount);
		ret = vkGetSwapchainImagesKHR(mDevice->GetHandle(), mHandle, &swapchainImageCount, mImages.data());
		return ret == VK_SUCCESS;
	}

	// 设置交换链的表面能力
	void AdVKSwapchain::SetupSurfaceCapabilities() {
		// 获取物理设备的表面能力
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &mSurfaceInfo.capabilities);

		// 获取设备的设置
		AdVkSettings settings = mDevice->GetSettings();

		// 获取表面格式数量
		uint32_t formatCount;
		CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &formatCount, nullptr));
		if (formatCount == 0) {
			LOG_E("{0} : num of surface format is 0", __FUNCTION__);
			return;
		}

		// 根据格式数量创建格式数组并获取格式信息
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &formatCount, formats.data()));

		// 查找设备设置中指定的表面格式
		int32_t foundFormatIndex = -1;
		for (int i = 0; i < formatCount; i++) {
			if (formats[i].format == settings.surfaceFormat && formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
				foundFormatIndex = i;
				break;
			}
		}

		// 如果未找到指定格式，则使用第一个格式
		if (foundFormatIndex == -1) {
			foundFormatIndex = 0;
		}
		mSurfaceInfo.surfaceFormat = formats[foundFormatIndex];

		// 获取表面呈现模式数量
		uint32_t presentModeCount;
		CALL_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &presentModeCount, nullptr));
		if (presentModeCount == 0) {
			LOG_E("{0} : num of surface present mode is 0", __FUNCTION__);
			return;
		}

		// 根据呈现模式数量创建呈现模式数组并获取呈现模式信息
		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		CALL_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &presentModeCount, presentModes.data()));

		// 查找设备设置中指定的呈现模式
		VkPresentModeKHR preferredPresentMode = settings.presentMode;
		int32_t foundPresentModeIndex = -1;
		for (int i = 0; i < presentModeCount; i++) {
			if (presentModes[i] == preferredPresentMode) {
				foundPresentModeIndex = i;
				break;
			}
		}

		// 如果找到指定的呈现模式，则使用它；否则使用第一个呈现模式
		if (foundPresentModeIndex >= 0) {
			mSurfaceInfo.presentMode = presentModes[foundPresentModeIndex];
		}
		else {
			mSurfaceInfo.presentMode = presentModes[0];
		}
	}

	/**
	* @brief 获取交换链中的下一个可用图像
	*
	* 该函数通过vkAcquireNextImageKHR从交换链中获取一个可用的图像，并可选地使用信号量或围栏来同步
	*
	* @param outImageIndex 指向一个整数的指针，用于存储获取的图像索引
	* @param semaphore 可选的信号量，用于在图像获取后发出信号
	* @param fence 可选的围栏，用于在图像获取后发出信号
	* @return VkResult 返回获取图像的结果，可能为VK_SUCCESS、VK_SUBOPTIMAL_KHR或其他错误代码
	*/
	VkResult AdVKSwapchain::AcquireImage(int32_t* outImageIndex, VkSemaphore semaphore, VkFence fence) {
		uint32_t imageIndex;
		// 调用vkAcquireNextImageKHR获取下一个可用图像的索引
		VkResult ret = vkAcquireNextImageKHR(mDevice->GetHandle(), mHandle, UINT64_MAX, semaphore, fence, &imageIndex);

		// 如果提供了围栏，则等待围栏被触发并重置围栏
		if (fence != VK_NULL_HANDLE) {
			CALL_VK(vkWaitForFences(mDevice->GetHandle(), 1, &fence, VK_FALSE, UINT64_MAX));
			CALL_VK(vkResetFences(mDevice->GetHandle(), 1, &fence));
		}

		// 如果成功获取了图像或图像状态为次优，更新外部和内部的当前图像索引
		if (ret == VK_SUCCESS || ret == VK_SUBOPTIMAL_KHR) {
			*outImageIndex = imageIndex;
			mCurrentImageIndex = imageIndex;
		}

		// 返回获取图像的结果
		return ret;
	}

	VkResult AdVKSwapchain::Present(int32_t imageIndex, const std::vector<VkSemaphore>& waitSemaphores) {
		VkPresentInfoKHR presentInfo = {
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			presentInfo.pNext = nullptr,
			presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
			presentInfo.pWaitSemaphores = waitSemaphores.data(),
			presentInfo.swapchainCount = 1,
			presentInfo.pSwapchains = &mHandle,
			presentInfo.pImageIndices = reinterpret_cast<const uint32_t*>(&imageIndex)
		};
		VkResult ret = vkQueuePresentKHR(mDevice->GetFirstPresentQueue()->GetHandle(), &presentInfo);
		mDevice->GetFirstPresentQueue()->WaitIdle();
		return ret;
	}
}