#include "Graphic/AdVKRenderPass.h"
#include "Graphic/AdVKDevice.h"
#include "Graphic/AdVKFrameBuffer.h"
#include "Adlog.h"

namespace WuDu {
	// 构造函数：AdVKRenderPass
	// 描述：初始化渲染通道，包括附件和子通道的设置
	// 参数：
	// - device: 设备指针，用于创建渲染通道
	// - attachments: 附件列表，定义了渲染过程中使用的图像资源
	// - subPasses: 子通道列表，定义了渲染流程和依赖关系
	AdVKRenderPass::AdVKRenderPass(AdVKDevice* device, const std::vector<Attachment>& attachments, const std::vector<RenderSubPass>& subPasses)
		: mDevice(device), mAttachments(attachments), mSubPasses(subPasses) {


		// 如果没有子通道配置，则设置默认的子通道和附件
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

		// 准备子通道描述、附件引用等数据结构
		// 根据子通道的数量初始化子通道描述向量
		std::vector<VkSubpassDescription> subpassDescriptions(mSubPasses.size());
		// 初始化输入附件引用的二维向量，每个子通道可以有多个输入附件
		std::vector<std::vector<VkAttachmentReference>> inputAttachmentRefs(mSubPasses.size());
		// 初始化颜色附件引用的二维向量，每个子通道可以有多个颜色附件
		std::vector<std::vector<VkAttachmentReference>> colorAttachmentRefs(mSubPasses.size());
		// 初始化深度模板附件引用的二维向量，每个子通道可以有多个深度模板附件
		std::vector<std::vector<VkAttachmentReference>> depthStencilAttachmentRefs(mSubPasses.size());
		// 初始化解析附件引用的向量，每个子通道可以有一个解析附件
		std::vector<VkAttachmentReference> resolveAttachmentRefs(mSubPasses.size());

		for (int i = 0; i < mSubPasses.size(); i++) {
			RenderSubPass subpass = mSubPasses[i];

			// 设置输入附件引用
			for (const auto& inputAttachment : subpass.inputAttachments) {
				inputAttachmentRefs[i].push_back({ inputAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
			}

			// 设置颜色附件引用，并根据样本数设置附件属性
			for (const auto& colorAttachment : subpass.colorAttachments) {
				colorAttachmentRefs[i].push_back({ colorAttachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
				mAttachments[colorAttachment].samples = subpass.sampleCount;
				if (subpass.sampleCount > VK_SAMPLE_COUNT_1_BIT) {
					mAttachments[colorAttachment].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				}
			}

			// 设置深度/模板附件引用，并根据样本数设置附件属性
			for (const auto& depthStencilAttachment : subpass.depthStencilAttachments) {
				depthStencilAttachmentRefs[i].push_back({ depthStencilAttachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });
				mAttachments[depthStencilAttachment].samples = subpass.sampleCount;
			}

			// 如果样本数大于1，添加解析附件
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

			// 填充子通道描述结构
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

		// 设置子通道间的依赖关系
		// 根据子通道数量初始化依赖向量，大小为子通道数量减一
		std::vector<VkSubpassDependency> dependencies(mSubPasses.size() - 1);

		// 当子通道数量大于1时，配置子通道间的依赖关系
		if (mSubPasses.size() > 1) {
			for (int i = 0; i < dependencies.size(); i++) {
				// 设置当前子通道为依赖的源子通道
				dependencies[i].srcSubpass = i;
				// 设置下一个子通道为依赖的目标子通道
				dependencies[i].dstSubpass = i + 1;
				// 源子通道的管道阶段为颜色附件输出阶段
				dependencies[i].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				// 目标子通道的管道阶段为片段着色器阶段
				dependencies[i].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				// 源子通道的访问类型为颜色附件写入
				dependencies[i].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				// 目标子通道的访问类型为输入附件读取
				dependencies[i].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
				// 设置依赖标志为按区域依赖
				dependencies[i].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			}
		}

		// 准备渲染通道创建信息
		// 创建一个存储VkAttachmentDescription对象的向量，用于描述渲染附件的属性
		std::vector<VkAttachmentDescription> vkAttachments;

		// 预先分配足够的空间，以提高内存使用效率和性能
		vkAttachments.reserve(mAttachments.size());

		// 遍历mAttachments集合中的每个附件描述对象
		for (const auto& attachment : mAttachments) {
			// 将当前附件的属性转换并复制到VkAttachmentDescription结构体中
			vkAttachments.push_back({
			    .flags = 0, // 保留字段，当前未使用
			    .format = attachment.format, // 指定附件的格式
			    .samples = attachment.samples, // 指定附件的样本数，用于多采样抗锯齿
			    .loadOp = attachment.loadOp, // 指定在渲染pass开始时如何处理附件的内容
			    .storeOp = attachment.storeOp, // 指定在渲染pass结束时如何处理附件的内容
			    .stencilLoadOp = attachment.stencilLoadOp, // 指定在渲染pass开始时如何处理模板附件的内容
			    .stencilStoreOp = attachment.stencilStoreOp, // 指定在渲染pass结束时如何处理模板附件的内容
			    .initialLayout = attachment.initialLayout, // 指定附件在渲染pass开始时的布局
			    .finalLayout = attachment.finalLayout // 指定附件在渲染pass结束时的布局
			});

		}
		
		// 初始化VkRenderPassCreateInfo结构体，用于描述渲染流程的创建信息
		VkRenderPassCreateInfo renderPassInfo = {
			// 指定结构体类型，这里是渲染流程创建信息
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			// 链接下一个结构体，当前无需链接其他结构体
			.pNext = nullptr,
			// 保留字段，目前未使用
			.flags = 0,
			// 设置渲染流程中使用的附件数量
			.attachmentCount = static_cast<uint32_t>(vkAttachments.size()),
			// 指向附件描述数组，定义了渲染流程中使用的颜色、深度或 stencil 附件
			.pAttachments = vkAttachments.data(),
			// 设置子流程的数量
			.subpassCount = static_cast<uint32_t>(mSubPasses.size()),
			// 指向子流程描述数组，定义了渲染流程中每个子流程的行为
			.pSubpasses = subpassDescriptions.data(),
			// 设置依赖关系的数量
			.dependencyCount = static_cast<uint32_t>(dependencies.size()),
			// 指向依赖关系描述数组，定义了子流程之间的依赖关系，以确保正确的执行顺序
			.pDependencies = dependencies.data()
		};
		// 创建渲染通道
		CALL_VK(vkCreateRenderPass(mDevice->GetHandle(), &renderPassInfo, nullptr, &mHandle));
		// 日志输出渲染通道创建信息
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