#ifndef ADVKFRAMEBUFFER_H
#define ADVKFRAMEBUFFER_H

#include "Graphic/AdVKCommon.h"
#include "Graphic/AdVKImageView.h"

namespace WuDu {
	class AdVKDevice;
	class AdVKRenderPass;
	class AdVKImage;

	class AdVKFrameBuffer {
	public:
		AdVKFrameBuffer(AdVKDevice* device, AdVKRenderPass* renderPass, const std::vector<std::shared_ptr<AdVKImage>>& images, uint32_t width, uint32_t height);
		~AdVKFrameBuffer();

		bool ReCreate(const std::vector<std::shared_ptr<AdVKImage>>& images, uint32_t width, uint32_t height);

		VkFramebuffer GetHandle() const { return mHandle; }
		uint32_t GetWidth() const { return mWidth; }
		uint32_t GetHeight() const { return mHeight; }

		// 按索引获取附件的 VkImageView 句柄
		VkImageView GetAttachmentImageView(uint32_t index) const {
			return mImageViews[index]->GetHandle();
		}
	private:
		VkFramebuffer mHandle = VK_NULL_HANDLE;
		AdVKDevice* mDevice;
		AdVKRenderPass* mRenderPass;
		uint32_t mWidth;
		uint32_t mHeight;
		std::vector<std::shared_ptr<AdVKImage>> mImages;
		std::vector<std::shared_ptr<AdVKImageView>> mImageViews;
	};
}

#endif