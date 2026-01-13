#ifndef AD_VKSWAPCHAIN_H
#define AD_VKSWAPCHAIN_H

#include "AdVKCommon.h"

namespace WuDu {
	class AdVKGraphicContext;
	class AdVKDevice;

	/**
	* 结构体SurfaceInfo用于存储交换链相关信息，包括能力、格式和呈现模式
	*/
	struct SurfaceInfo {
		VkSurfaceCapabilitiesKHR capabilities; /**< 存储交换链的能力，包括尺寸和图像数量 */
		VkSurfaceFormatKHR surfaceFormat;     /**< 交换链的格式，包括颜色空间和像素格式 */
		VkPresentModeKHR presentMode;         /**< 指定交换链的呈现模式，如即时更新、FIFO等 */
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

		// 创建一个SurfaceInfo结构体的实例mSurfaceInfo，用于存储交换链的信息
		SurfaceInfo mSurfaceInfo;
	};
}

#endif