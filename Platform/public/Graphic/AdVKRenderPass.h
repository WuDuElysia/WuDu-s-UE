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
	* RenderSubPass结构体用于描述渲染流程中的一个子通道，
	* 包含了该子通道所需的输入附件、颜色附件和深度模板附件，
	* 以及渲染样本计数。
	*/
	struct RenderSubPass {
		// 输入附件索引数组，用于指定子通道所需的输入纹理
		std::vector<uint32_t> inputAttachments;

		// 颜色附件索引数组，用于指定子通道渲染输出的颜色缓冲区
		std::vector<uint32_t> colorAttachments;

		// 深度模板附件索引数组，用于指定子通道渲染输出的深度模板缓冲区
		std::vector<uint32_t> depthStencilAttachments;

		// 渲染样本计数，默认为单样本
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