#ifndef AD_VKSWAPCHAIN_H
#define AD_VKSWAPCHAIN_H

#include "AdVKCommon.h"

namespace ade {
	class AdVKGraphicContext;
	class AdVKDevice;

	/**
	* 结构体SurfaceInfo用于存储表面的相关信息，包括表面的能力、格式和呈现模式
	*/
	struct SurfaceInfo {
		VkSurfaceCapabilitiesKHR capabilities; /**< 存储表面的能力，如尺寸和图像数量限制 */
		VkSurfaceFormatKHR surfaceFormat;     /**< 定义表面的格式，包括颜色空间和像素格式 */
		VkPresentModeKHR presentMode;         /**< 指定表面的呈现模式，如邮件箱、FIFO等 */
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

		// 定义一个SurfaceInfo类型的变量mSurfaceInfo，用于存储表面相关信息
		SurfaceInfo mSurfaceInfo;
	};
}

#endif