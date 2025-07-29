#include "Graphic/AdVKRenderPass.h"
#include "Graphic/AdVKDevice.h"
#include "Graphic/AdVKFrameBuffer.h"

namespace ade {
	// ���캯����AdVKRenderPass
	// ��������ʼ����Ⱦͨ����������������ͨ��������
	// ������
	// - device: �豸ָ�룬���ڴ�����Ⱦͨ��
	// - attachments: �����б���������Ⱦ������ʹ�õ�ͼ����Դ
	// - subPasses: ��ͨ���б���������Ⱦ���̺�������ϵ
	AdVKRenderPass::AdVKRenderPass(AdVKDevice* device, const std::vector<Attachment>& attachments, const std::vector<RenderSubPass>& subPasses)
		: mDevice(device), mAttachments(attachments), mSubPasses(subPasses) {

		// ���û����ͨ�����ã�������Ĭ�ϵ���ͨ���͸���
		if (mSubPasses.empty()) {
			mAttachments = { {
				.format = device->GetSettings().surfaceFormat,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
			    } };
			mSubPasses = { {.colorAttachments = { 0 }, .sampleCount = VK_SAMPLE_COUNT_1_BIT } };
		}

		// ׼����ͨ���������������õ����ݽṹ
		// ������ͨ����������ʼ����ͨ����������
		std::vector<VkSubpassDescription> subpassDescriptions(mSubPasses.size());
		// ��ʼ�����븽�����õĶ�ά������ÿ����ͨ�������ж�����븽��
		std::vector<std::vector<VkAttachmentReference>> inputAttachmentRefs(mSubPasses.size());
		// ��ʼ����ɫ�������õĶ�ά������ÿ����ͨ�������ж����ɫ����
		std::vector<std::vector<VkAttachmentReference>> colorAttachmentRefs(mSubPasses.size());
		// ��ʼ�����ģ�帽�����õĶ�ά������ÿ����ͨ�������ж�����ģ�帽��
		std::vector<std::vector<VkAttachmentReference>> depthStencilAttachmentRefs(mSubPasses.size());
		// ��ʼ�������������õ�������ÿ����ͨ��������һ����������
		std::vector<VkAttachmentReference> resolveAttachmentRefs(mSubPasses.size());

		for (int i = 0; i < mSubPasses.size(); i++) {
			RenderSubPass subpass = mSubPasses[i];

			// �������븽������
			for (const auto& inputAttachment : subpass.inputAttachments) {
				inputAttachmentRefs[i].push_back({ inputAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
			}

			// ������ɫ�������ã����������������ø�������
			for (const auto& colorAttachment : subpass.colorAttachments) {
				colorAttachmentRefs[i].push_back({ colorAttachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
				mAttachments[colorAttachment].samples = subpass.sampleCount;
				if (subpass.sampleCount > VK_SAMPLE_COUNT_1_BIT) {
					mAttachments[colorAttachment].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				}
			}

			// �������/ģ�帽�����ã����������������ø�������
			for (const auto& depthStencilAttachment : subpass.depthStencilAttachments) {
				depthStencilAttachmentRefs[i].push_back({ depthStencilAttachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });
				mAttachments[depthStencilAttachment].samples = subpass.sampleCount;
			}

			// �������������1����ӽ�������
			if (subpass.sampleCount > VK_SAMPLE_COUNT_1_BIT) {
				mAttachments.emplace_back(
					device->GetSettings().surfaceFormat,
					VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					VK_ATTACHMENT_STORE_OP_STORE,
					VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					VK_ATTACHMENT_STORE_OP_DONT_CARE,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					VK_SAMPLE_COUNT_1_BIT,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
				);
				resolveAttachmentRefs[i] = { static_cast<uint32_t>(mAttachments.size() - 1), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
			}

			// �����ͨ�������ṹ
			subpassDescriptions[i].flags = 0;
			subpassDescriptions[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassDescriptions[i].inputAttachmentCount = inputAttachmentRefs[i].size();
			subpassDescriptions[i].pInputAttachments = inputAttachmentRefs[i].data();
			subpassDescriptions[i].colorAttachmentCount = colorAttachmentRefs[i].size();
			subpassDescriptions[i].pColorAttachments = colorAttachmentRefs[i].data();
			subpassDescriptions[i].pResolveAttachments = subpass.sampleCount > VK_SAMPLE_COUNT_1_BIT ? &resolveAttachmentRefs[i] : nullptr;
			subpassDescriptions[i].pDepthStencilAttachment = depthStencilAttachmentRefs[i].data();
			subpassDescriptions[i].preserveAttachmentCount = 0;
			subpassDescriptions[i].pPreserveAttachments = nullptr;
		}

		// ������ͨ�����������ϵ
		// ������ͨ��������ʼ��������������СΪ��ͨ��������һ
		std::vector<VkSubpassDependency> dependencies(mSubPasses.size() - 1);

		// ����ͨ����������1ʱ��������ͨ�����������ϵ
		if (mSubPasses.size() > 1) {
			for (int i = 0; i < dependencies.size(); i++) {
				// ���õ�ǰ��ͨ��Ϊ������Դ��ͨ��
				dependencies[i].srcSubpass = i;
				// ������һ����ͨ��Ϊ������Ŀ����ͨ��
				dependencies[i].dstSubpass = i + 1;
				// Դ��ͨ���Ĺܵ��׶�Ϊ��ɫ��������׶�
				dependencies[i].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				// Ŀ����ͨ���Ĺܵ��׶�ΪƬ����ɫ���׶�
				dependencies[i].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				// Դ��ͨ���ķ�������Ϊ��ɫ����д��
				dependencies[i].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				// Ŀ����ͨ���ķ�������Ϊ���븽����ȡ
				dependencies[i].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
				// ����������־Ϊ����������
				dependencies[i].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			}
		}

		// ׼����Ⱦͨ��������Ϣ
		// ����һ���洢VkAttachmentDescription���������������������Ⱦ����������
		std::vector<VkAttachmentDescription> vkAttachments;

		// Ԥ�ȷ����㹻�Ŀռ䣬������ڴ�ʹ��Ч�ʺ�����
		vkAttachments.reserve(mAttachments.size());

		// ����mAttachments�����е�ÿ��������������
		for (const auto& attachment : mAttachments) {
			// ����ǰ����������ת�������Ƶ�VkAttachmentDescription�ṹ����
			vkAttachments.push_back({
			    .flags = 0, // �����ֶΣ���ǰδʹ��
			    .format = attachment.format, // ָ�������ĸ�ʽ
			    .samples = attachment.samples, // ָ�������������������ڶ���������
			    .loadOp = attachment.loadOp, // ָ������Ⱦpass��ʼʱ��δ�����������
			    .storeOp = attachment.storeOp, // ָ������Ⱦpass����ʱ��δ�����������
			    .stencilLoadOp = attachment.stencilLoadOp, // ָ������Ⱦpass��ʼʱ��δ���ģ�帽��������
			    .stencilStoreOp = attachment.stencilStoreOp, // ָ������Ⱦpass����ʱ��δ���ģ�帽��������
			    .initialLayout = attachment.initialLayout, // ָ����������Ⱦpass��ʼʱ�Ĳ���
			    .finalLayout = attachment.finalLayout // ָ����������Ⱦpass����ʱ�Ĳ���
				});
		}
		// ��ʼ��VkRenderPassCreateInfo�ṹ�壬����������Ⱦ���̵Ĵ�����Ϣ
		VkRenderPassCreateInfo renderPassInfo = {
			// ָ���ṹ�����ͣ���������Ⱦ���̴�����Ϣ
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			// ������һ���ṹ�壬��ǰ�������������ṹ��
			.pNext = nullptr,
			// �����ֶΣ�Ŀǰδʹ��
			.flags = 0,
			// ������Ⱦ������ʹ�õĸ�������
			.attachmentCount = static_cast<uint32_t>(vkAttachments.size()),
			// ָ�򸽼��������飬��������Ⱦ������ʹ�õ���ɫ����Ȼ� stencil ����
			.pAttachments = vkAttachments.data(),
			// ���������̵�����
			.subpassCount = static_cast<uint32_t>(mSubPasses.size()),
			// ָ���������������飬��������Ⱦ������ÿ�������̵���Ϊ
			.pSubpasses = subpassDescriptions.data(),
			// ����������ϵ������
			.dependencyCount = static_cast<uint32_t>(dependencies.size()),
			// ָ��������ϵ�������飬������������֮���������ϵ����ȷ����ȷ��ִ��˳��
			.pDependencies = dependencies.data()
		};
		// ������Ⱦͨ��
		CALL_VK(vkCreateRenderPass(mDevice->GetHandle(), &renderPassInfo, nullptr, &mHandle));
		// ��־�����Ⱦͨ��������Ϣ
		LOG_T("RenderPass {0} : {1}, attachment count: {2}, subpass count: {3}", __FUNCTION__, (void*)mHandle, mAttachments.size(), mSubPasses.size());
	}

	AdVKRenderPass::~AdVKRenderPass() {
		VK_D(RenderPass, mDevice->GetHandle(), mHandle);
	}

	void AdVKRenderPass::Begin(VkCommandBuffer cmdBuffer, AdVKFrameBuffer* frameBuffer, const std::vector<VkClearValue>& clearValues) const {
		VkRect2D renderArea = {
			.offset = { 0, 0 },
			.extent = { frameBuffer->GetWidth(), frameBuffer->GetHeight() }
		};
		VkRenderPassBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = mHandle,
			.framebuffer = frameBuffer->GetHandle(),
			.renderArea = renderArea,
			.clearValueCount = static_cast<uint32_t>(clearValues.size()),
			.pClearValues = clearValues.data()
		};
		vkCmdBeginRenderPass(cmdBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void AdVKRenderPass::End(VkCommandBuffer cmdBuffer) const {
		vkCmdEndRenderPass(cmdBuffer);
	}
}