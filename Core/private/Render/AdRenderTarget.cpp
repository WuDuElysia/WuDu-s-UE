#include "Render/AdRenderTarget.h"
#include "AdApplication.h"
#include "Graphic/AdVKRenderPass.h"
#include "Graphic/AdVKImage.h"
#include "ECS/Component/AdLookAtCameraComponent.h"
#include "ECS/Component/AdFirstPersonCameraComponent.h"

namespace ade {

	/**
	* @brief AdRenderTarget���캯�������ڴ�����ȾĿ�����
	* @param renderPass ָ��Vulkan��Ⱦͨ�������ָ�룬����ָ����ȾĿ�����Ⱦͨ��
	*/
	AdRenderTarget::AdRenderTarget(AdVKRenderPass* renderPass) {
		// ��ȡ��Ⱦ�����ĺͽ���������
		AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		// ��ʼ����ȾĿ��Ļ�������
		mRenderPass = renderPass;
		mBufferCount = swapchain->GetImages().size();
		mExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
		bSwapchainTarget = true;

		// ִ�г�ʼ�������´�������
		Init();
		ReCreate();
	}

	/**
	 * @brief ���캯�������ڴ����Զ�����ȾĿ����󣨷ǽ������󶨣�
	 * @param renderPass ��Ⱦͨ������ָ��
	 * @param bufferCount ֡��������
	 * @param extent ��Ⱦ�����С����ߣ�
	 */
	AdRenderTarget::AdRenderTarget(AdVKRenderPass* renderPass, uint32_t bufferCount, VkExtent2D extent) :
		mRenderPass(renderPass), mBufferCount(bufferCount), mExtent(extent), bSwapchainTarget(false) {
		Init();
		ReCreate();
	}

	/**
	 * @brief �����������ͷ����в���ϵͳ��Դ
	 */
	AdRenderTarget::~AdRenderTarget() {
		for (const auto& item : mMaterialSystemList) {
			item->OnDestroy();
		}
		mMaterialSystemList.clear();
	}

	/**
	 * @brief ��ʼ����ȾĿ������ֵ����
	 */
	void AdRenderTarget::Init() {
		mClearValues.resize(mRenderPass->GetAttachmentSize());
		SetColorClearValue({ 0.f, 0.f, 0.f, 1.f });
		SetDepthStencilClearValue({ 1.f, 0 });
	}

	/**
 * @brief ���´���֡���弰���ͼ����Դ
 * 
 * �ú������ݵ�ǰ��ȾĿ��ĳߴ����Ⱦͨ���������ã����´���֡���弰�������ͼ����Դ��
 * �����ȾĿ��ߴ�Ϊ0����ֱ�ӷ��ز�ִ���κβ�����
 * ����ÿ��������������ݸ������;����Ƿ�ʹ�ý�����ͼ��򴴽�����ͼ��
 * ����Ϊÿ��������������Ӧ��֡�������
 */
void AdRenderTarget::ReCreate() {
	// �����Ȼ�߶�Ϊ0�������д�������
	if (mExtent.width == 0 || mExtent.height == 0) {
		return;
	}

	// ��ղ���������֡���������С
	mFrameBuffers.clear();
	mFrameBuffers.resize(mBufferCount);

	// ��ȡ��Ⱦ��������ض���
	AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
	AdVKDevice* device = renderCxt->GetDevice();
	AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

	// ��ȡ��Ⱦͨ����������
	std::vector<Attachment> attachments = mRenderPass->GetAttachments();
	if (attachments.empty()) {
		return;
	}

	// ��ȡ������ͼ���б�
	std::vector<VkImage> swapchainImages = swapchain->GetImages();

	// ����ÿ����������������Ӧ��ͼ����Դ��֡����
	for (int i = 0; i < mBufferCount; i++) {
		std::vector<std::shared_ptr<AdVKImage>> images;

		// ���ݸ������ô���ͼ����Դ
		for (int j = 0; j < attachments.size(); j++) {
			Attachment attachment = attachments[j];

			// �ж��Ƿ�ʹ�ý�����ͼ��
			if (bSwapchainTarget && attachment.finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && attachment.samples == VK_SAMPLE_COUNT_1_BIT) {
				// ʹ�ý�����ͼ�񴴽�ͼ����Դ
				images.push_back(std::make_shared<AdVKImage>(device, swapchainImages[i], VkExtent3D{ mExtent.width, mExtent.height, 1 }, attachment.format, attachment.usage));
			}
			else {
				// ��������ͼ����Դ
				images.push_back(std::make_shared<AdVKImage>(device, VkExtent3D{ mExtent.width, mExtent.height, 1 }, attachment.format, attachment.usage, attachment.samples));
			}
		}

		// ����֡�������
		mFrameBuffers[i] = std::make_shared<AdVKFrameBuffer>(device, mRenderPass, images, mExtent.width, mExtent.height);
		images.clear();
	}
}

/**
* @brief ��ʼ��ȾĿ�����Ⱦ����
*
* @param cmdBuffer Vulkan������������ڼ�¼��Ⱦ����
*
* �ú�������׼����ȾĿ�����Ⱦ�������������״̬��������Դ��
* ���������������ȡ��ǰ�����������Լ���ʼ��Ⱦͨ���Ȳ�����
*/
void AdRenderTarget::Begin(VkCommandBuffer cmdBuffer) {
	assert(!bBeginTarget && "You should not called Begin() again.");

	// �����Ҫ���£������´�����ȾĿ����Դ
	if (bShouldUpdate) {
		ReCreate();
		bShouldUpdate = false;
	}

	

	// �����Ƿ�Ϊ������Ŀ����ȷ����ǰʹ�õĻ���������
	if (bSwapchainTarget) {
		AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		AdVKSwapchain* swapchain = renderCxt->GetSwapchain();
		mCurrentBufferIdx = swapchain->GetCurrentImageIndex();
	}
	else {
		mCurrentBufferIdx = (mCurrentBufferIdx + 1) % mBufferCount;
	}

	// ��ʼ��Ⱦͨ��
	mRenderPass->Begin(cmdBuffer, GetFrameBuffer(), mClearValues);
	bBeginTarget = true;
}

	/**
	 * @brief ������ȾĿ�����Ⱦ����
	 * @param cmdBuffer Vulkan����������
	 */
	void AdRenderTarget::End(VkCommandBuffer cmdBuffer) {
		if (bBeginTarget) {
			mRenderPass->End(cmdBuffer);
			bBeginTarget = false;
		}
	}

	/**
	 * @brief ������ȾĿ��ĳߴ�
	 * @param extent �µ���Ⱦ�����С
	 */
	void AdRenderTarget::SetExtent(const VkExtent2D& extent) {
		mExtent = extent;
		bShouldUpdate = true;
	}

	/**
	 * @brief ����֡��������
	 * @param bufferCount �µ�֡��������
	 */
	void AdRenderTarget::SetBufferCount(uint32_t bufferCount) {
		mBufferCount = bufferCount;
		bShouldUpdate = true;
	}

	/**
	 * @brief ����������ɫ�����������ɫֵ
	 * @param colorClearValue ��ɫ���ֵ�ṹ��
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
	 * @brief �����������/ģ�帽�������ֵ
	 * @param depthStencilValue ���/ģ�����ֵ�ṹ��
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
	 * @brief ����ָ����������ɫ���������ɫֵ
	 * @param attachmentIndex ��������
	 * @param colorClearValue ��ɫ���ֵ�ṹ��
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
	 * @brief ����ָ�����������/ģ�帽�����ֵ
	 * @param attachmentIndex ��������
	 * @param depthStencilValue ���/ģ�����ֵ�ṹ��
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