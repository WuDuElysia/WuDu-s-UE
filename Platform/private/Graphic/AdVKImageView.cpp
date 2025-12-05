#include "Graphic/AdVKImageView.h"
#include "Graphic/AdVKDevice.h"

namespace WuDu {
	/**
	 * @brief 构造函数，创建Vulkan图像视图对象
	 * @param device 指向Vulkan设备对象的指针，用于创建图像视图
	 * @param image Vulkan图像对象句柄，指定要创建视图的图像
	 * @param format 图像格式，指定图像视图的像素格式
	 * @param aspectFlags 图像方面标志，指定图像视图包含的图像方面（如颜色、深度等）
	 * @note 该构造函数会调用Vulkan API创建图像视图，并将句柄存储在mHandle成员中
	 */
	AdVKImageView::AdVKImageView(AdVKDevice* device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) : mDevice(device) {
		// 配置图像视图创建信息结构体
		VkImageViewCreateInfo imageViewInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.components = {
				VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = {
				.aspectMask = aspectFlags,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};
		// 调用Vulkan API创建图像视图
		CALL_VK(vkCreateImageView(device->GetHandle(), &imageViewInfo, nullptr, &mHandle));
	}

	/**
	 * @brief 析构函数，销毁Vulkan图像视图对象
	 * @note 该析构函数会调用Vulkan API销毁图像视图，释放相关资源
	 */
	AdVKImageView::~AdVKImageView() {
		VK_D(ImageView, mDevice->GetHandle(), mHandle);
	}
}