#include "Graphic/AdVKImage.h"
#include "Graphic/AdVKDevice.h"
#include "Graphic/AdVKBuffer.h"

namespace ade {
	/**
 * @brief ���캯�������ڴ��� Vulkan ͼ�����
 *
 * �ù��캯������ݴ���Ĳ�������һ�� Vulkan ͼ�񣬲�Ϊ������豸�ڴ档
 * ����ͼ���ʽ�Ƿ�Ϊ���/ģ���ʽ���Ƿ����ö��ز�����ѡ����ʵ�ͼ�񲼾֣�tiling����
 *
 * @param device ָ�� Vulkan �豸�����ָ�룬���ڴ���ͼ��ͷ����ڴ档
 * @param extent ͼ��ĳߴ磨��ȡ��߶ȡ���ȣ���
 * @param format ͼ������ظ�ʽ��
 * @param usage ͼ���ʹ�ñ�־��ָ��ͼ�񽫱����ʹ�ã�����ɫ�����������ȣ���
 * @param sampleCount ͼ��Ĳ����������ڶ��ز�������ݡ�
 */
	AdVKImage::AdVKImage(AdVKDevice* device, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount) : mDevice(device),
		mExtent(extent),
		mFormat(format),
		mUsage(usage) {
		// ����ͼ���ʽ�Ͳ���������ͼ����ڴ沼�֣����Ի����ţ�
		VkImageTiling tiling = VK_IMAGE_TILING_LINEAR;
		bool isDepthStencilFormat = IsDepthStencilFormat(format);
		if (isDepthStencilFormat || sampleCount > VK_SAMPLE_COUNT_1_BIT) {
			tiling = VK_IMAGE_TILING_OPTIMAL;
		}

		// ���ͼ�񴴽���Ϣ�ṹ��
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

		// ���� Vulkan ͼ�����
		CALL_VK(vkCreateImage(mDevice->GetHandle(), &imageInfo, nullptr, &mHandle));

		// ��ȡͼ���ڴ�����
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(mDevice->GetHandle(), mHandle, &memReqs);

		// ����ڴ������Ϣ�ṹ��
		VkMemoryAllocateInfo allocateInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = memReqs.size,
			.memoryTypeIndex = static_cast<uint32_t>(mDevice->GetMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memReqs.memoryTypeBits))
		};

		// �����豸�ڴ沢�󶨵�ͼ�����
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

	void AdVKImage::CopyFromBuffer(VkCommandBuffer cmdBuffer, AdVKBuffer* buffer) {
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
		vkCmdCopyBufferToImage(cmdBuffer, buffer->GetHandle(), mHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	/**
 * @brief ת�� Vulkan ͼ��Ĳ��֣�Image Layout Transition��
 *
 * �ú��������� Vulkan �н�ͼ���һ������ת��Ϊ��һ�����֡��������Դ���ֺ�Ŀ�겼���Զ������ڴ����ϣ�VkImageMemoryBarrier����
 * ������ vkCmdPipelineBarrier ��������������ָ������������С�
 *
 * @param cmdBuffer ������������ڼ�¼ͼ�񲼾�ת���Ĺ�����������
 * @param image ��Ҫ���в���ת���� Vulkan ͼ�������
 * @param oldLayout ��ǰͼ��Ĳ���
 * @param newLayout Ŀ��ͼ��Ĳ���
 * @return ����ɹ�����������ϲ���ɲ���ת���򷵻� true�����򷵻� false
 */
	bool AdVKImage::TransitionLayout(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
		// ���ͼ�����Ƿ���Ч
		if (image == VK_NULL_HANDLE) {
			return false;
		}
		// ����¾ɲ�����ͬ��������ת����ֱ�ӷ��سɹ�
		if (oldLayout == newLayout) {
			return true;
		}

		// ��ʼ��ͼ���ڴ����Ͻṹ��
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

		// ����Ĭ�ϵĹ��߽׶α�־
		VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		// ���ݾɲ�������Դ��������
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

		// �����²�������Ŀ��������룬�����ܸ���Դ��������
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

		// �����������������ִ��ͼ�񲼾�ת��
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