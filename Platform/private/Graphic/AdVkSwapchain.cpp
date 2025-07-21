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

	// ���´���������
	bool AdVKSwapchain::ReCreate() {
		// �ָ���־����������Ķ�
		LOG_D("-----------------------------");
		// ���ñ�������
		SetupSurfaceCapabilities();
		// ��־�����ǰ��չ�������ʽ�ͳ���ģʽ
		LOG_D("currentExtent : {0} x {1}", mSurfaceInfo.capabilities.currentExtent.width, mSurfaceInfo.capabilities.currentExtent.height);
		LOG_D("surfaceFormat : {0}", vk_format_string(mSurfaceInfo.surfaceFormat.format));
		LOG_D("presentMode   : {0}", vk_present_mode_string(mSurfaceInfo.presentMode));
		LOG_D("-----------------------------");

		// ��ʼ��ͼ������
		uint32_t imageCount = mDevice->GetSettings().swapchainImageCount;
		// ȷ��ͼ�������ڱ�����������С�����ͼ������֮��
		if (imageCount < mSurfaceInfo.capabilities.minImageCount && mSurfaceInfo.capabilities.minImageCount > 0) {
			imageCount = mSurfaceInfo.capabilities.minImageCount;
		}
		if (imageCount > mSurfaceInfo.capabilities.maxImageCount && mSurfaceInfo.capabilities.maxImageCount > 0) {
			imageCount = mSurfaceInfo.capabilities.maxImageCount;
		}

		// ȷ��ͼ����ģʽ�Ͷ��м�ͥ����
		/*��δ����������� Vulkan ͼ��Ĺ���ģʽ����صĶ��м������������幦�����£�
		���ͼ�κͳ��ֶ��м�����ͬ����ʹ�ö�ռģʽ��VK_SHARING_MODE_EXCLUSIVE��������ָ�����м���������
		����ʹ�ò���ģʽ��VK_SHARING_MODE_CONCURRENT������ָ��������ͬ�Ķ��м����������ֱ�����ͼ�κͳ��ֲ�����*/
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

		// ����ɵĽ��������
		VkSwapchainKHR oldSwapchain = mHandle;

		// ��佻����������Ϣ�ṹ��
		VkSwapchainCreateInfoKHR swapchainInfo = {
			// ָ���ṹ������
			swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			// ָ����һ���ṹ���ָ�룬�˴�����Ҫ���������ṹ��
			swapchainInfo.pNext = nullptr,
			// �����ֶΣ�����δ��ʹ�ã�ĿǰӦ����Ϊ0
			swapchainInfo.flags = 0,
			// ������������ı���
			swapchainInfo.surface = mContext->GetSurface(),
			// ��Сͼ��������Ӱ����Ⱦ���ܺ��ڴ�ʹ��
			swapchainInfo.minImageCount = imageCount,
			// ͼ��ĸ�ʽ
			swapchainInfo.imageFormat = mSurfaceInfo.surfaceFormat.format,
			// ͼ�����ɫ�ռ�
			swapchainInfo.imageColorSpace = mSurfaceInfo.surfaceFormat.colorSpace,
			// ͼ��ķֱ���
			swapchainInfo.imageExtent = mSurfaceInfo.capabilities.currentExtent,
			// ͼ��Ĳ�����ͨ��Ϊ1������2D��Ⱦ
			swapchainInfo.imageArrayLayers = 1,
			// ͼ���ʹ�÷�ʽ���˴���Ϊ��ɫ����
			swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			// ͼ����ģʽ����������ڶ���֮�乲��ͼ��
			swapchainInfo.imageSharingMode = imageSharingMode,
			// ��������������������֧��ͼ����ģʽ
			swapchainInfo.queueFamilyIndexCount = queueFamilyIndexCount,
			// ����������ָ�룬ָ������ͼ����Ķ�����
			swapchainInfo.pQueueFamilyIndices = pQueueFamilyIndices,
			// ����任��ʽ���˴�����Ϊ�ޱ任
			swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			// ���alphaֵ���˴�����Ϊ�̳�
			swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			//swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
			// ����ģʽ��Ӱ����Ⱦ�������Ժ��ӳ�
			swapchainInfo.presentMode = mSurfaceInfo.presentMode,
			// �Ƿ�����ͼ�񱻲ü���������Ҫ���ֵ���������
			swapchainInfo.clipped = VK_FALSE,
			// �ɵĽ����������ڽ��������ؽ�
			swapchainInfo.oldSwapchain = oldSwapchain
		};
		// ����������
		VkResult ret = vkCreateSwapchainKHR(mDevice->GetHandle(), &swapchainInfo, nullptr, &mHandle);
		if (ret != VK_SUCCESS) {
			LOG_E("{0} : {1}", __FUNCTION__, vk_result_string(ret));
			return false;
		}
		// ��־���������������Ϣ
		LOG_T("Swapchain {0} : old: {1}, new: {2}, image count: {3}, format: {4}, present mode : {5}", __FUNCTION__, (void*)oldSwapchain, (void*)mHandle, imageCount,
			vk_format_string(mSurfaceInfo.surfaceFormat.format), vk_present_mode_string(mSurfaceInfo.presentMode));

		// ��ȡ������ͼ������������ͼ�������С
		uint32_t swapchainImageCount;
		ret = vkGetSwapchainImagesKHR(mDevice->GetHandle(), mHandle, &swapchainImageCount, nullptr);
		mImages.resize(swapchainImageCount);
		ret = vkGetSwapchainImagesKHR(mDevice->GetHandle(), mHandle, &swapchainImageCount, mImages.data());
		return ret == VK_SUCCESS;
	}

	// ���ý������ı�������
	void AdVKSwapchain::SetupSurfaceCapabilities() {
		// ��ȡ�����豸�ı�������
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &mSurfaceInfo.capabilities);

		// ��ȡ�豸������
		AdVkSettings settings = mDevice->GetSettings();

		// ��ȡ�����ʽ����
		uint32_t formatCount;
		CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &formatCount, nullptr));
		if (formatCount == 0) {
			LOG_E("{0} : num of surface format is 0", __FUNCTION__);
			return;
		}

		// ���ݸ�ʽ����������ʽ���鲢��ȡ��ʽ��Ϣ
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &formatCount, formats.data()));

		// �����豸������ָ���ı����ʽ
		int32_t foundFormatIndex = -1;
		for (int i = 0; i < formatCount; i++) {
			if (formats[i].format == settings.surfaceFormat && formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
				foundFormatIndex = i;
				break;
			}
		}

		// ���δ�ҵ�ָ����ʽ����ʹ�õ�һ����ʽ
		if (foundFormatIndex == -1) {
			foundFormatIndex = 0;
		}
		mSurfaceInfo.surfaceFormat = formats[foundFormatIndex];

		// ��ȡ�������ģʽ����
		uint32_t presentModeCount;
		CALL_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &presentModeCount, nullptr));
		if (presentModeCount == 0) {
			LOG_E("{0} : num of surface present mode is 0", __FUNCTION__);
			return;
		}

		// ���ݳ���ģʽ������������ģʽ���鲢��ȡ����ģʽ��Ϣ
		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		CALL_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(mContext->GetPhyDevice(), mContext->GetSurface(), &presentModeCount, presentModes.data()));

		// �����豸������ָ���ĳ���ģʽ
		VkPresentModeKHR preferredPresentMode = settings.presentMode;
		int32_t foundPresentModeIndex = -1;
		for (int i = 0; i < presentModeCount; i++) {
			if (presentModes[i] == preferredPresentMode) {
				foundPresentModeIndex = i;
				break;
			}
		}

		// ����ҵ�ָ���ĳ���ģʽ����ʹ����������ʹ�õ�һ������ģʽ
		if (foundPresentModeIndex >= 0) {
			mSurfaceInfo.presentMode = presentModes[foundPresentModeIndex];
		}
		else {
			mSurfaceInfo.presentMode = presentModes[0];
		}
	}

	/**
	* @brief ��ȡ�������е���һ������ͼ��
	*
	* �ú���ͨ��vkAcquireNextImageKHR�ӽ������л�ȡһ�����õ�ͼ�񣬲���ѡ��ʹ���ź�����Χ����ͬ��
	*
	* @param outImageIndex ָ��һ��������ָ�룬���ڴ洢��ȡ��ͼ������
	* @param semaphore ��ѡ���ź�����������ͼ���ȡ�󷢳��ź�
	* @param fence ��ѡ��Χ����������ͼ���ȡ�󷢳��ź�
	* @return VkResult ���ػ�ȡͼ��Ľ��������ΪVK_SUCCESS��VK_SUBOPTIMAL_KHR�������������
	*/
	VkResult AdVKSwapchain::AcquireImage(int32_t* outImageIndex, VkSemaphore semaphore, VkFence fence) {
		uint32_t imageIndex;
		// ����vkAcquireNextImageKHR��ȡ��һ������ͼ�������
		VkResult ret = vkAcquireNextImageKHR(mDevice->GetHandle(), mHandle, UINT64_MAX, semaphore, fence, &imageIndex);

		// ����ṩ��Χ������ȴ�Χ��������������Χ��
		if (fence != VK_NULL_HANDLE) {
			CALL_VK(vkWaitForFences(mDevice->GetHandle(), 1, &fence, VK_FALSE, UINT64_MAX));
			CALL_VK(vkResetFences(mDevice->GetHandle(), 1, &fence));
		}

		// ����ɹ���ȡ��ͼ���ͼ��״̬Ϊ���ţ������ⲿ���ڲ��ĵ�ǰͼ������
		if (ret == VK_SUCCESS || ret == VK_SUBOPTIMAL_KHR) {
			*outImageIndex = imageIndex;
			mCurrentImageIndex = imageIndex;
		}

		// ���ػ�ȡͼ��Ľ��
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