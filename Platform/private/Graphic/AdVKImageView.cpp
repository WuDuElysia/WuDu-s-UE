#include "Graphic/AdVKImageView.h"
#include "Graphic/AdVKDevice.h"

namespace ade {
	/**
	 * @brief ���캯��������Vulkanͼ����ͼ����
	 * @param device ָ��Vulkan�豸�����ָ�룬���ڴ���ͼ����ͼ
	 * @param image Vulkanͼ���������ָ��Ҫ������ͼ��ͼ��
	 * @param format ͼ���ʽ��ָ��ͼ����ͼ�����ظ�ʽ
	 * @param aspectFlags ͼ�����־��ָ��ͼ����ͼ������ͼ���棨����ɫ����ȵȣ�
	 * @note �ù��캯�������Vulkan API����ͼ����ͼ����������洢��mHandle��Ա��
	 */
	AdVKImageView::AdVKImageView(AdVKDevice* device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) : mDevice(device) {
		// ����ͼ����ͼ������Ϣ�ṹ��
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
		// ����Vulkan API����ͼ����ͼ
		CALL_VK(vkCreateImageView(device->GetHandle(), &imageViewInfo, nullptr, &mHandle));
	}

	/**
	 * @brief ��������������Vulkanͼ����ͼ����
	 * @note ���������������Vulkan API����ͼ����ͼ���ͷ������Դ
	 */
	AdVKImageView::~AdVKImageView() {
		VK_D(ImageView, mDevice->GetHandle(), mHandle);
	}
}