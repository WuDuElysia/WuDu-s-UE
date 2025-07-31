#ifndef AD_VKSWAPCHAIN_H
#define AD_VKSWAPCHAIN_H

#include "AdVKCommon.h"

namespace ade {
	class AdVKGraphicContext;
	class AdVKDevice;

	/**
	* �ṹ��SurfaceInfo���ڴ洢����������Ϣ�������������������ʽ�ͳ���ģʽ
	*/
	struct SurfaceInfo {
		VkSurfaceCapabilitiesKHR capabilities; /**< �洢�������������ߴ��ͼ���������� */
		VkSurfaceFormatKHR surfaceFormat;     /**< �������ĸ�ʽ��������ɫ�ռ�����ظ�ʽ */
		VkPresentModeKHR presentMode;         /**< ָ������ĳ���ģʽ�����ʼ��䡢FIFO�� */
	};

	class AdVKSwapchain {
	public:
		AdVKSwapchain(AdVKGraphicContext* context, AdVKDevice* device);
		~AdVKSwapchain();

		bool ReCreate();

		VkResult AcquireImage(uint32_t& outImageIndex, VkSemaphore semaphore, VkFence fence = VK_NULL_HANDLE);
		VkResult Present(int32_t imageIndex, const std::vector<VkSemaphore>& waitSemaphores);

		const std::vector<VkImage>& GetImages() const { return mImages; }
		uint32_t GetWidth() const { return mSurfaceInfo.capabilities.currentExtent.width; }
		uint32_t GetHeight() const { return mSurfaceInfo.capabilities.currentExtent.height; }
		int32_t GetCurrentImageIndex() const { return mCurrentImageIndex; }

		const SurfaceInfo& GetSurfaceInfo() const { return mSurfaceInfo; }
	private:
		void SetupSurfaceCapabilities();

		VkSwapchainKHR mHandle = VK_NULL_HANDLE;

		AdVKGraphicContext* mContext;
		AdVKDevice* mDevice;
		std::vector<VkImage> mImages;

		int32_t mCurrentImageIndex = -1;

		// ����һ��SurfaceInfo���͵ı���mSurfaceInfo�����ڴ洢���������Ϣ
		SurfaceInfo mSurfaceInfo;
	};
}

#endif