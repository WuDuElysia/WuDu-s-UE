#include "Graphic/AdVKFrameBuffer.h"
#include "Graphic/AdVKDevice.h"
#include "Graphic/AdVKRenderPass.h"
#include "Graphic/AdVKImage.h"
#include "Graphic/AdVKImageView.h"

namespace ade {
	AdVKFrameBuffer::AdVKFrameBuffer(AdVKDevice* device, AdVKRenderPass* renderPass, const std::vector<std::shared_ptr<AdVKImage>>& images, uint32_t width, uint32_t height)
		: mDevice(device), mRenderPass(renderPass), mImages(images), mWidth(width), mHeight(height) {
		ReCreate(images, width, height);
	}

	AdVKFrameBuffer::~AdVKFrameBuffer() {
		VK_D(Framebuffer, mDevice->GetHandle(), mHandle);
	}

	/**
	* @brief ���´���֡�������
	* @param images ���ڴ���֡�����ͼ����Դ�б�
	* @param width ֡����Ŀ��
	* @param height ֡����ĸ߶�
	* @return �����ɹ�����true��ʧ�ܷ���false
	*
	* �ú���������ԭ�е�֡������󣬸��ݴ����ͼ����Դ���´����µ�֡���塣
	* ֧����ɫ��������ȸ������Զ�ʶ��ʹ���
	*/
	bool AdVKFrameBuffer::ReCreate(const std::vector<std::shared_ptr<AdVKImage>>& images, uint32_t width, uint32_t height) {
		VkResult ret;

		// ����ԭ�е�֡�������
		VK_D(Framebuffer, mDevice->GetHandle(), mHandle);

		mWidth = width;
		mHeight = height;
		mImageViews.clear();

		// ����������ͼ�б�����ͼ���ʽ�Զ��ж�����ȸ���������ɫ����
		std::vector<VkImageView> attachments(images.size());
		for (int i = 0; i < images.size(); i++) {
			bool isDepthFormat = IsDepthOnlyFormat(images[i]->GetFormat()); // FIXME when format is stencil format
			mImageViews.push_back(std::make_shared<AdVKImageView>(mDevice, images[i]->GetHandle(),
				images[i]->GetFormat(), isDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT));
			attachments[i] = mImageViews[i]->GetHandle();
		}

		// ���ò�����֡�������
		VkFramebufferCreateInfo frameBufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = mRenderPass->GetHandle(),
			.attachmentCount = static_cast<uint32_t>(mImageViews.size()),
			.pAttachments = attachments.data(),
			.width = width,
			.height = height,
			.layers = 1
		};
		ret = vkCreateFramebuffer(mDevice->GetHandle(), &frameBufferInfo, nullptr, &mHandle);
		LOG_T("FrameBuffer {0}, new: {1}, width: {2}, height: {3}, view count: {4}", __FUNCTION__, (void*)mHandle, mWidth, mHeight, mImageViews.size());
		return ret == VK_SUCCESS;
	}
}