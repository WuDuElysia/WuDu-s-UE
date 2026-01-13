#ifndef AD_VKRENDERPASS_H
#define AD_VKRENDERPASS_H

#include "Graphic/AdVKCommon.h"

namespace WuDu {
	class AdVKDevice;
	class AdVKFrameBuffer;

	struct Attachment {
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
		VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	};

	/**
	* @brief 渲染子通道结构体
	*
	* RenderSubPass结构体表示渲染通道中的一个子通道
	* 包含了该子通道的输入附件、颜色附件、深度/模板附件
	* 以及渲染子通道的样本数
	*/
	struct RenderSubPass {
		// 输入附件索引列表，指定子通道的输入附件索引
		std::vector<uint32_t> inputAttachments;

		// 颜色附件索引列表，指定子通道渲染的颜色附件
		std::vector<uint32_t> colorAttachments;

		// 深度模板附件索引列表，指定子通道渲染的深度模板附件
		std::vector<uint32_t> depthStencilAttachments;

		// 渲染子通道的样本数，默认为1个样本
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
	};

	class AdVKRenderPass {
	public:
		AdVKRenderPass(AdVKDevice* device, const std::vector<Attachment>& attachments = {}, const std::vector<RenderSubPass>& subPasses = {});
		~AdVKRenderPass();

		VkRenderPass GetHandle() const { return mHandle; }

		void Begin(VkCommandBuffer cmdBuffer, AdVKFrameBuffer* frameBuffer, const std::vector<VkClearValue>& clearValues) const;
		void End(VkCommandBuffer cmdBuffer) const;

		const std::vector<Attachment>& GetAttachments() const { return mAttachments; }
		uint32_t GetAttachmentSize() const { return mAttachments.size(); }
		const std::vector<RenderSubPass>& GetSubPasses() const { return mSubPasses; }
	private:
		VkRenderPass mHandle = VK_NULL_HANDLE;
		AdVKDevice* mDevice;

		std::vector<Attachment> mAttachments;
		std::vector<RenderSubPass> mSubPasses;
	};
}

#endif