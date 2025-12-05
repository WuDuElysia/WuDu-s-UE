#include "Render/AdRenderTarget.h"
#include "AdApplication.h"
#include "Graphic/AdVKRenderPass.h"
#include "Graphic/AdVKImage.h"
#include "ECS/Component/AdLookAtCameraComponent.h"
#include "ECS/Component/AdFirstPersonCameraComponent.h"

namespace WuDu {

	/**
	* @brief AdRenderTarget构造函数，用于创建渲染目标对象
	* @param renderPass 指向Vulkan渲染通道对象的指针，用于指定渲染目标的渲染通道
	*/
	AdRenderTarget::AdRenderTarget(AdVKRenderPass* renderPass) {
		// 获取渲染上下文和交换链对象
		AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		// 初始化渲染目标的基本属性
		mRenderPass = renderPass;
		mBufferCount = swapchain->GetImages().size();
		mExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
		bSwapchainTarget = true;

		// 执行初始化和重新创建操作
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
 * 
 * 该函数根据当前渲染目标的尺寸和渲染通道附件配置，重新创建帧缓冲及其关联的图像资源。
 * 如果渲染目标尺寸为0，则直接返回不执行任何操作。
 * 对于每个缓冲区，会根据附件类型决定是否使用交换链图像或创建离屏图像。
 * 最终为每个缓冲区创建对应的帧缓冲对象。
 */
void AdRenderTarget::ReCreate() {
	// 如果宽度或高度为0，不进行创建操作
	if (mExtent.width == 0 || mExtent.height == 0) {
		LOG_W("RenderTarget ReCreate() skipped: invalid extent {0}x{1}", mExtent.width, mExtent.height);
		return;
	}

	// 获取渲染上下文相关对象
	AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
	AdVKDevice* device = renderCxt->GetDevice();
	AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

	// 如果是交换链目标，确保使用最新的交换链信息
	if (bSwapchainTarget) {
		// 获取当前交换链的最新信息
		uint32_t currentWidth = swapchain->GetWidth();
		uint32_t currentHeight = swapchain->GetHeight();
		uint32_t currentImageCount = swapchain->GetImages().size();
		
		// 记录交换链状态，用于调试
		LOG_D("Swapchain Target ReCreate - Width: {0}, Height: {1}, Image Count: {2}", 
			currentWidth, currentHeight, currentImageCount);
		
		// 更新渲染目标尺寸和缓冲数量以匹配当前交换链
		mExtent = { currentWidth, currentHeight };
		mBufferCount = currentImageCount;
	}

	// 清空并重新设置帧缓冲数组大小
	mFrameBuffers.clear();
	mFrameBuffers.resize(mBufferCount);

	// 获取渲染通道附件配置
	std::vector<Attachment> attachments = mRenderPass->GetAttachments();
	if (attachments.empty()) {
		LOG_W("RenderTarget ReCreate() skipped: no attachments");
		return;
	}

	// 获取交换链图像列表（只在需要时获取）
	std::vector<VkImage> swapchainImages;
	if (bSwapchainTarget) {
		swapchainImages = swapchain->GetImages();
		// 确保交换链图像数量与期望的缓冲数量匹配
			if (swapchainImages.size() != mBufferCount) {
				LOG_E("Swapchain image count ({0}) mismatch with buffer count ({1})", 
				swapchainImages.size(), mBufferCount);
				return;
			}
	}

	// 遍历每个缓冲区，创建对应的图像资源和帧缓冲
	for (int i = 0; i < mBufferCount; i++) {
		std::vector<std::shared_ptr<AdVKImage>> images;

		// 根据附件配置创建图像资源
		for (int j = 0; j < attachments.size(); j++) {
			Attachment attachment = attachments[j];

			// 判断是否使用交换链图像
			if (bSwapchainTarget && attachment.finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && attachment.samples == VK_SAMPLE_COUNT_1_BIT) {
				// 安全检查：确保索引在有效范围内
				if (i >= static_cast<int>(swapchainImages.size())) {
					LOG_E("Invalid swapchain image index: {0}, total images: {1}", 
						i, swapchainImages.size());
					return;
				}
				
				// 使用交换链图像创建图像资源
				LOG_T("Creating framebuffer {0}, using swapchain image {1} for attachment {2}", 
					i, i, j);
				images.push_back(std::make_shared<AdVKImage>(device, swapchainImages[i], VkExtent3D{ mExtent.width, mExtent.height, 1 }, attachment.format, attachment.usage));
			} else {
				// 创建离屏图像资源（深度/模板附件等）
				LOG_T("Creating framebuffer {0}, creating offscreen image for attachment {1}", 
					i, j);
				images.push_back(std::make_shared<AdVKImage>(device, VkExtent3D{ mExtent.width, mExtent.height, 1 }, attachment.format, attachment.usage, attachment.samples));
			}
		}

		// 创建帧缓冲对象
		LOG_T("Creating framebuffer {0} with {1} attachments", i, images.size());
		mFrameBuffers[i] = std::make_shared<AdVKFrameBuffer>(device, mRenderPass, images, mExtent.width, mExtent.height);
		images.clear();
	}
	
	LOG_D("RenderTarget ReCreate() completed: {0} framebuffers created", mFrameBuffers.size());
}

/**
* @brief 开始渲染目标的渲染过程
*
* @param cmdBuffer Vulkan命令缓冲区，用于记录渲染命令
*
* 该函数负责准备渲染目标的渲染环境，包括检查状态、更新资源、
* 设置相机参数、获取当前缓冲区索引以及开始渲染通道等操作。
*/
void AdRenderTarget::Begin(VkCommandBuffer cmdBuffer) {
	assert(!bBeginTarget && "You should not called Begin() again.");

	// 获取渲染上下文和交换链
	AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
	AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

	// 对于交换链目标，总是检查并确保帧缓冲与当前交换链匹配
	if (bSwapchainTarget) {
		// 获取当前交换链的状态
		uint32_t currentSwapchainWidth = swapchain->GetWidth();
		uint32_t currentSwapchainHeight = swapchain->GetHeight();
		uint32_t currentSwapchainImageCount = swapchain->GetImages().size();
		
		// 如果帧缓冲数量与当前交换链图像数量不匹配，或者尺寸不匹配，或者需要更新，则重新创建帧缓冲
		if (mBufferCount != currentSwapchainImageCount || 
			mExtent.width != currentSwapchainWidth || 
			mExtent.height != currentSwapchainHeight || 
			bShouldUpdate) {
			ReCreate();
			bShouldUpdate = false;
		}
		
		// 获取当前图像索引，并确保它在有效范围内
		mCurrentBufferIdx = swapchain->GetCurrentImageIndex();
		// 安全检查：确保索引有效
		if (mCurrentBufferIdx < 0 || mCurrentBufferIdx >= static_cast<int32_t>(mFrameBuffers.size())) {
			LOG_W("Invalid current image index: {0}, frame buffer count: {1}", mCurrentBufferIdx, mFrameBuffers.size());
			mCurrentBufferIdx = 0;
		}
	} else {
		// 对于非交换链目标，使用循环索引
		mCurrentBufferIdx = (mCurrentBufferIdx + 1) % mBufferCount;
		// 如果需要更新，则重新创建渲染目标资源
		if (bShouldUpdate) {
			ReCreate();
			bShouldUpdate = false;
		}
	}

	// 安全检查：确保当前帧缓冲存在且有效
	if (mCurrentBufferIdx >= 0 && mCurrentBufferIdx < static_cast<int32_t>(mFrameBuffers.size())) {
		// 开始渲染通道
		mRenderPass->Begin(cmdBuffer, GetFrameBuffer(), mClearValues);
		bBeginTarget = true;
	} else {
		LOG_E("Failed to begin render pass: invalid frame buffer index {0}", mCurrentBufferIdx);
	}
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
	// 无论尺寸是否变化，只要调用了SetExtent，就标记为需要更新
	// 这样可以确保在交换链重建时，即使尺寸相同，也会重建帧缓冲
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