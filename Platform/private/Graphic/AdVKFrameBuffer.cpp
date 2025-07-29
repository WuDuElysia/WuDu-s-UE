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
	* @brief 重新创建帧缓冲对象
	* @param images 用于创建帧缓冲的图像资源列表
	* @param width 帧缓冲的宽度
	* @param height 帧缓冲的高度
	* @return 创建成功返回true，失败返回false
	*
	* 该函数会销毁原有的帧缓冲对象，根据传入的图像资源重新创建新的帧缓冲。
	* 支持颜色附件和深度附件的自动识别和处理。
	*/
	bool AdVKFrameBuffer::ReCreate(const std::vector<std::shared_ptr<AdVKImage>>& images, uint32_t width, uint32_t height) {
		VkResult ret;

		// 销毁原有的帧缓冲对象
		VK_D(Framebuffer, mDevice->GetHandle(), mHandle);

		mWidth = width;
		mHeight = height;
		mImageViews.clear();

		// 创建附件视图列表，根据图像格式自动判断是深度附件还是颜色附件
		std::vector<VkImageView> attachments(images.size());
		for (int i = 0; i < images.size(); i++) {
			bool isDepthFormat = IsDepthOnlyFormat(images[i]->GetFormat()); // FIXME when format is stencil format
			mImageViews.push_back(std::make_shared<AdVKImageView>(mDevice, images[i]->GetHandle(),
				images[i]->GetFormat(), isDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT));
			attachments[i] = mImageViews[i]->GetHandle();
		}

		// 配置并创建帧缓冲对象
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