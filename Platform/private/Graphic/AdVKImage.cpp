#include "Graphic/AdVKImage.h"
#include "Graphic/AdVKDevice.h"
#include "Graphic/AdVKBuffer.h"

namespace WuDu {
	/**
 * @brief 构造函数，用于创建 Vulkan 图像对象。
 *
 * 该构造函数会根据传入的参数创建一个 Vulkan 图像，并为其分配设备内存。
 * 根据图像格式是否为深度/模板格式或是否启用多重采样，选择合适的图像布局（tiling）。
 *
 * @param device 指向 Vulkan 设备对象的指针，用于创建图像和分配内存。
 * @param extent 图像的尺寸（宽度、高度、深度）。
 * @param format 图像的像素格式。
 * @param usage 图像的使用标志，指定图像将被如何使用（如颜色附件、采样等）。
 * @param sampleCount 图像的采样数，用于多重采样抗锯齿。
 */
	AdVKImage::AdVKImage(AdVKDevice* device, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount) : mDevice(device),
		mExtent(extent),
		mFormat(format),
		mUsage(usage) {
		// 根据图像格式和采样数决定图像的内存布局（线性或最优）
		VkImageTiling tiling = VK_IMAGE_TILING_LINEAR;
		bool isDepthStencilFormat = IsDepthStencilFormat(format);
		if (isDepthStencilFormat || sampleCount > VK_SAMPLE_COUNT_1_BIT) {
			tiling = VK_IMAGE_TILING_OPTIMAL;
		}

		// 填充图像创建信息结构体
		VkImageCreateInfo imageInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = format,
			.extent = extent,
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = sampleCount,
			.tiling = tiling,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};

		// 创建 Vulkan 图像对象
		CALL_VK(vkCreateImage(mDevice->GetHandle(), &imageInfo, nullptr, &mHandle));

		// 获取图像内存需求
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(mDevice->GetHandle(), mHandle, &memReqs);

		// 填充内存分配信息结构体
		VkMemoryAllocateInfo allocateInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = memReqs.size,
			.memoryTypeIndex = static_cast<uint32_t>(mDevice->GetMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memReqs.memoryTypeBits))
		};

		// 分配设备内存并绑定到图像对象
		CALL_VK(vkAllocateMemory(mDevice->GetHandle(), &allocateInfo, nullptr, &mMemory));
		CALL_VK(vkBindImageMemory(mDevice->GetHandle(), mHandle, mMemory, 0));
	}

	AdVKImage::AdVKImage(AdVKDevice* device, VkImage image, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount)
		: mHandle(image), mDevice(device), mExtent(extent), mFormat(format), mUsage(usage), bCreateImage(false) {
	}


	AdVKImage::~AdVKImage() {
		if (bCreateImage) {
			VK_D(Image, mDevice->GetHandle(), mHandle);
			VK_F(mDevice->GetHandle(), mMemory);
		}
	}

	/**
 * @brief 将缓冲区数据复制到图像
 *
 * 该函数将指定缓冲区中的数据复制到当前图像对象中，通常用于纹理加载等场景
 *
 * @param cmdBuffer Vulkan命令缓冲区句柄，用于记录复制命令
 * @param buffer 源缓冲区对象指针，包含要复制到图像的数据
 */
	void AdVKImage::CopyFromBuffer(VkCommandBuffer cmdBuffer, AdVKBuffer* buffer) {
		// 配置缓冲区到图像的复制区域参数
		VkBufferImageCopy region = {
		    .bufferOffset = 0,
		    .bufferRowLength = mExtent.width,
		    .bufferImageHeight = mExtent.height,
		    .imageSubresource = {
			    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			    .mipLevel = 0,
			    .baseArrayLayer = 0,
			    .layerCount = 1
		    },
		    .imageOffset = { 0, 0, 0 },
		    .imageExtent = { mExtent.width, mExtent.height, 1 }
		};
		// 执行缓冲区到图像的数据复制操作
		vkCmdCopyBufferToImage(cmdBuffer, buffer->GetHandle(), mHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	/**
 * @brief 转换 Vulkan 图像的布局（Image Layout Transition）
 *
 * 该函数用于在 Vulkan 中将图像从一个布局转换为另一个布局。它会根据源布局和目标布局自动设置内存屏障（VkImageMemoryBarrier），
 * 并调用 vkCmdPipelineBarrier 插入管线屏障命令到指定的命令缓冲区中。
 *
 * @param cmdBuffer 命令缓冲区，用于记录图像布局转换的管线屏障命令
 * @param image 需要进行布局转换的 Vulkan 图像对象句柄
 * @param oldLayout 当前图像的布局
 * @param newLayout 目标图像的布局
 * @return 如果成功插入管线屏障并完成布局转换则返回 true，否则返回 false
 */
	bool AdVKImage::TransitionLayout(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
		// 检查图像句柄是否有效
		if (image == VK_NULL_HANDLE) {
			return false;
		}
		// 如果新旧布局相同，则无需转换，直接返回成功
		if (oldLayout == newLayout) {
			return true;
		}

		// 初始化图像内存屏障结构体
		VkImageMemoryBarrier barrier;
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext = nullptr;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		// 设置默认的管线阶段标志
		VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		// 根据旧布局设置源访问掩码
		switch (oldLayout) {
		case VK_IMAGE_LAYOUT_UNDEFINED:
			barrier.srcAccessMask = 0;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			LOG_E("Unsupported layout transition : {0} --> {1}", vk_image_layout_string(oldLayout), vk_image_layout_string(newLayout));
			return false;
		}

		// 根据新布局设置目标访问掩码，并可能更新源访问掩码
		switch (newLayout) {
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			barrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			if (barrier.srcAccessMask == 0)
			{
				barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			LOG_E("Unsupported layout transition : {0} --> {1}", vk_image_layout_string(oldLayout), vk_image_layout_string(newLayout));
			return false;
		}

		// 插入管线屏障命令以执行图像布局转换
		vkCmdPipelineBarrier(
			cmdBuffer,
			srcStage, dstStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
		return true;
	}
}