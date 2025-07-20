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
		    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		    swapchainInfo.pNext = nullptr,
		    swapchainInfo.flags = 0,
		    swapchainInfo.surface = mContext->GetSurface(),
		    swapchainInfo.minImageCount = imageCount,
		    swapchainInfo.imageFormat = mSurfaceInfo.surfaceFormat.format,
		    swapchainInfo.imageColorSpace = mSurfaceInfo.surfaceFormat.colorSpace,
		    swapchainInfo.imageExtent = mSurfaceInfo.capabilities.currentExtent,
		    swapchainInfo.imageArrayLayers = 1,
		    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		    swapchainInfo.imageSharingMode = imageSharingMode,
		    swapchainInfo.queueFamilyIndexCount = queueFamilyIndexCount,
		    swapchainInfo.pQueueFamilyIndices = pQueueFamilyIndices,
		    swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		    swapchainInfo.presentMode = mSurfaceInfo.presentMode,
		    swapchainInfo.clipped = VK_FALSE,
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

	void AdVKSwapchain::SetupSurfaceCapabilities() {
		// capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &mSurfaceInfo.capabilities);

		AdVkSettings settings = mDevice->GetSettings();

		// format
		uint32_t formatCount;
		CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &formatCount, nullptr));
		if (formatCount == 0) {
			LOG_E("{0} : num of surface format is 0", __FUNCTION__);
			return;
		}
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &formatCount, formats.data()));
		int32_t foundFormatIndex = -1;
		for (int i = 0; i < formatCount; i++) {
			if (formats[i].format == settings.surfaceFormat && formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
				foundFormatIndex = i;
				break;
			}
		}
		if (foundFormatIndex == -1) {
			foundFormatIndex = 0;
		}
		mSurfaceInfo.surfaceFormat = formats[foundFormatIndex];

		// present mode
		uint32_t presentModeCount;
		CALL_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &presentModeCount, nullptr));
		if (presentModeCount == 0) {
			LOG_E("{0} : num of surface present mode is 0", __FUNCTION__);
			return;
		}
		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		CALL_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &presentModeCount, presentModes.data()));
		VkPresentModeKHR preferredPresentMode = mDevice->GetSettings().presentMode;
		int32_t foundPresentModeIndex = -1;
		for (int i = 0; i < presentModeCount; i++) {
			if (presentModes[i] == preferredPresentMode) {
				foundPresentModeIndex = i;
				break;
			}
		}
		if (foundPresentModeIndex >= 0) {
			mSurfaceInfo.presentMode = presentModes[foundPresentModeIndex];
		}
		else {
			mSurfaceInfo.presentMode = presentModes[0];
		}
	}

	VkResult AdVKSwapchain::AcquireImage(int32_t* outImageIndex, VkSemaphore semaphore, VkFence fence) {
		uint32_t imageIndex;
		VkResult ret = vkAcquireNextImageKHR(mDevice->GetHandle(), mHandle, UINT64_MAX, semaphore, fence, &imageIndex);
		if (fence != VK_NULL_HANDLE) {
			CALL_VK(vkWaitForFences(mDevice->GetHandle(), 1, &fence, VK_FALSE, UINT64_MAX));
			CALL_VK(vkResetFences(mDevice->GetHandle(), 1, &fence));
		}

		if (ret == VK_SUCCESS || ret == VK_SUBOPTIMAL_KHR) {
			*outImageIndex = imageIndex;
			mCurrentImageIndex = imageIndex;
		}
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