#include "Render/AdRenderTarget.h"
#include "AdApplication.h"
#include "Graphic/AdVKRenderPass.h"
#include "Graphic/AdVKImage.h"
#include "ECS/Component/AdLookAtCameraComponent.h"

namespace ade {

	/**
	 * @brief 构造函数，用于创建与交换链绑定的渲染目标对象
	 * @param renderPass 渲染通道对象指针，用于描述渲染流程中的附件和子通道信息
	 */
	AdRenderTarget::AdRenderTarget(AdVKRenderPass* renderPass) {
		AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		mRenderPass = renderPass;
		mBufferCount = swapchain->GetImages().size();
		mExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
		bSwapchainTarget = true;

		Init();
		ReCreate();
	}

	/**
	 * @brief 构造函数，用于创建自定义渲染目标对象（非交换链绑定）
	 * @param renderPass 渲染通道对象指针
	 * @param bufferCount 帧缓冲数量
	 * @param extent 渲染区域大小（宽高）
	 */
	AdRenderTarget::AdRenderTarget(AdVKRenderPass* renderPass, uint32_t bufferCount, VkExtent2D extent) :
		mRenderPass(renderPass), mBufferCount(bufferCount), mExtent(extent), bSwapchainTarget(false) {
		Init();
		ReCreate();
	}

	/**
	 * @brief 析构函数，释放所有材质系统资源
	 */
	AdRenderTarget::~AdRenderTarget() {
		for (const auto& item : mMaterialSystemList) {
			item->OnDestroy();
		}
		mMaterialSystemList.clear();
	}

	/**
	 * @brief 初始化渲染目标的清除值配置
	 */
	void AdRenderTarget::Init() {
		mClearValues.resize(mRenderPass->GetAttachmentSize());
		SetColorClearValue({ 0.f, 0.f, 0.f, 1.f });
		SetDepthStencilClearValue({ 1.f, 0 });
	}

	/**
	 * @brief 重新创建帧缓冲及相关图像资源
	 */
	void AdRenderTarget::ReCreate() {
		if (mExtent.width == 0 || mExtent.height == 0) {
			return;
		}
		mFrameBuffers.clear();
		mFrameBuffers.resize(mBufferCount);

		AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		AdVKDevice* device = renderCxt->GetDevice();
		AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		std::vector<Attachment> attachments = mRenderPass->GetAttachments();
		if (attachments.empty()) {
			return;
		}

		std::vector<VkImage> swapchainImages = swapchain->GetImages();

		for (int i = 0; i < mBufferCount; i++) {
			std::vector<std::shared_ptr<AdVKImage>> images;
			for (int j = 0; j < attachments.size(); j++) {
				Attachment attachment = attachments[j];
				if (bSwapchainTarget && attachment.finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && attachment.samples == VK_SAMPLE_COUNT_1_BIT) {
					// 使用交换链图像创建图像资源
					images.push_back(std::make_shared<AdVKImage>(device, swapchainImages[i], VkExtent3D{ mExtent.width, mExtent.height, 1 }, attachment.format, attachment.usage));
				}
				else {
					// 创建离屏图像资源
					images.push_back(std::make_shared<AdVKImage>(device, VkExtent3D{ mExtent.width, mExtent.height, 1 }, attachment.format, attachment.usage, attachment.samples));
				}
			}
			mFrameBuffers[i] = std::make_shared<AdVKFrameBuffer>(device, mRenderPass, images, mExtent.width, mExtent.height);
			images.clear();
		}
	}

	/**
	 * @brief 开始渲染目标的渲染过程
	 * @param cmdBuffer Vulkan命令缓冲区句柄
	 */
	void AdRenderTarget::Begin(VkCommandBuffer cmdBuffer) {
		assert(!bBeginTarget && "You should not called Begin() again.");

		if (bShouldUpdate) {
			ReCreate();
			bShouldUpdate = false;
		}
		if (AdEntity::HasComponent<AdLookAtCameraComponent>(mCamera)) {
			mCamera->GetComponent<AdLookAtCameraComponent>().SetAspect(mExtent.width * 1.f / mExtent.height);
		}
		if (bSwapchainTarget) {
			AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
			AdVKSwapchain* swapchain = renderCxt->GetSwapchain();
			mCurrentBufferIdx = swapchain->GetCurrentImageIndex();
		}
		else {
			mCurrentBufferIdx = (mCurrentBufferIdx + 1) % mBufferCount;
		}

		mRenderPass->Begin(cmdBuffer, GetFrameBuffer(), mClearValues);
		bBeginTarget = true;
	}

	/**
	 * @brief 结束渲染目标的渲染过程
	 * @param cmdBuffer Vulkan命令缓冲区句柄
	 */
	void AdRenderTarget::End(VkCommandBuffer cmdBuffer) {
		if (bBeginTarget) {
			mRenderPass->End(cmdBuffer);
			bBeginTarget = false;
		}
	}

	/**
	 * @brief 设置渲染目标的尺寸
	 * @param extent 新的渲染区域大小
	 */
	void AdRenderTarget::SetExtent(const VkExtent2D& extent) {
		mExtent = extent;
		bShouldUpdate = true;
	}

	/**
	 * @brief 设置帧缓冲数量
	 * @param bufferCount 新的帧缓冲数量
	 */
	void AdRenderTarget::SetBufferCount(uint32_t bufferCount) {
		mBufferCount = bufferCount;
		bShouldUpdate = true;
	}

	/**
	 * @brief 设置所有颜色附件的清除颜色值
	 * @param colorClearValue 颜色清除值结构体
	 */
	void AdRenderTarget::SetColorClearValue(VkClearColorValue colorClearValue) {
		std::vector<Attachment> renderPassAttachments = mRenderPass->GetAttachments();
		for (int i = 0; i < renderPassAttachments.size(); i++) {
			if (!IsDepthStencilFormat(renderPassAttachments[i].format) && renderPassAttachments[i].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
				mClearValues[i].color = colorClearValue;
			}
		}
	}

	/**
	 * @brief 设置所有深度/模板附件的清除值
	 * @param depthStencilValue 深度/模板清除值结构体
	 */
	void AdRenderTarget::SetDepthStencilClearValue(VkClearDepthStencilValue depthStencilValue) {
		std::vector<Attachment> renderPassAttachments = mRenderPass->GetAttachments();
		for (int i = 0; i < renderPassAttachments.size(); i++) {
			if (IsDepthStencilFormat(renderPassAttachments[i].format) && renderPassAttachments[i].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
				mClearValues[i].depthStencil = depthStencilValue;
			}
		}
	}

	/**
	 * @brief 设置指定索引的颜色附件清除颜色值
	 * @param attachmentIndex 附件索引
	 * @param colorClearValue 颜色清除值结构体
	 */
	void AdRenderTarget::SetColorClearValue(uint32_t attachmentIndex, VkClearColorValue colorClearValue) {
		std::vector<Attachment> renderPassAttachments = mRenderPass->GetAttachments();
		if (attachmentIndex <= renderPassAttachments.size() - 1) {
			if (!IsDepthStencilFormat(renderPassAttachments[attachmentIndex].format) && renderPassAttachments[attachmentIndex].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
				mClearValues[attachmentIndex].color = colorClearValue;
			}
		}
	}

	/**
	 * @brief 设置指定索引的深度/模板附件清除值
	 * @param attachmentIndex 附件索引
	 * @param depthStencilValue 深度/模板清除值结构体
	 */
	void AdRenderTarget::SetDepthStencilClearValue(uint32_t attachmentIndex, VkClearDepthStencilValue depthStencilValue) {
		std::vector<Attachment> renderPassAttachments = mRenderPass->GetAttachments();
		if (attachmentIndex <= renderPassAttachments.size() - 1) {
			if (IsDepthStencilFormat(renderPassAttachments[attachmentIndex].format) && renderPassAttachments[attachmentIndex].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
				mClearValues[attachmentIndex].depthStencil = depthStencilValue;
			}
		}
	}
}