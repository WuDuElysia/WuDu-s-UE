#include "Render/AdRenderer.h"
#include "AdApplication.h"
#include "Graphic/AdVKQueue.h"

namespace ade {
	/**
 * @brief AdRenderer���캯��
 * @details ��ʼ�������Ⱦ������������Vulkanͬ�����ź�����դ��
 * @param ��
 * @return �޷���ֵ
 */
	AdRenderer::AdRenderer() {
		// ��ȡ��Ⱦ�����ĺ��豸���
		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		ade::AdVKDevice* device = renderCxt->GetDevice();

		// ����ͬ������������С��ƥ�仺��������
		mImageAvailableSemaphores.resize(RENDERER_NUM_BUFFER);
		mSubmitedSemaphores.resize(RENDERER_NUM_BUFFER);
		mFrameFences.resize(RENDERER_NUM_BUFFER);

		// �����ź���������Ϣ
		VkSemaphoreCreateInfo semaphoreInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};

		// ����դ��������Ϣ����ʼ״̬Ϊ�Ѵ���
		VkFenceCreateInfo fenceInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT

		};

		// Ϊÿ������������ͬ������ͼ������ź������ύ����ź�����֡դ����
		for (int i = 0; i < RENDERER_NUM_BUFFER; i++) {
			CALL_VK(vkCreateSemaphore(device->GetHandle(), &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]));
			CALL_VK(vkCreateSemaphore(device->GetHandle(), &semaphoreInfo, nullptr, &mSubmitedSemaphores[i]));
			CALL_VK(vkCreateFence(device->GetHandle(), &fenceInfo, nullptr, &mFrameFences[i]));
		}
	}

	AdRenderer::~AdRenderer() {
		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		ade::AdVKDevice* device = renderCxt->GetDevice();
		for (const auto& item : mImageAvailableSemaphores) {
			VK_D(Semaphore, device->GetHandle(), item);
		}
		for (const auto& item : mSubmitedSemaphores) {
			VK_D(Semaphore, device->GetHandle(), item);
		}
		for (const auto& item : mFrameFences) {
			VK_D(Fence, device->GetHandle(), item);
		}
	}

	/**
 * @brief ׼����ʼһ֡��Ⱦ����Ҫ���������ȴ���һ֡��ɡ���ȡ������ͼ��������
 *        ���ڱ�Ҫʱ�ؽ���������
 *
 * �ú�����ȴ���ǰ֡�� Fence �źţ�ȷ��ǰһ֡�������Ⱦ��Ȼ���Դӽ�������
 * ��ȡ��һ֡Ҫ��Ⱦ��ͼ�����������������ʧЧ���細�ڴ�С�ı䣩����᳢���ؽ�
 * ���������������Ƿ�ߴ緢���仯�����Ƿ���Ҫ������ȾĿ�ꡣ
 *
 * @param[out] outImageIndex ���ػ�ȡ���Ľ�����ͼ�����������ں�����Ⱦ���̡�
 *                           ������ nullptr���������ֵ��
 *
 * @return bool ������������ؽ��ҳߴ緢���仯������ true����ʾ�ϲ������Ҫ
 *              ������ص���ȾĿ����ӿ����ã����򷵻� false��
 */
	bool AdRenderer::Begin(int32_t* outImageIndex) {
		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		ade::AdVKDevice* device = renderCxt->GetDevice();
		ade::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();

		bool bShouldUpdateTarget = false;

		// �ȴ���ǰ֡�� Fence��ȷ��ǰһ֡�����
		CALL_VK(vkWaitForFences(device->GetHandle(), 1, &mFrameFences[mCurrentBuffer], VK_TRUE, UINT64_MAX));
		// ���� Fence��Ϊ��ǰ֡��׼��
		CALL_VK(vkResetFences(device->GetHandle(), 1, &mFrameFences[mCurrentBuffer]));

		// ����һ�� uint32_t ����������ͼ������
		uint32_t imageIndex;
		VkResult ret = swapchain->AcquireImage(imageIndex, mImageAvailableSemaphores[mCurrentBuffer]);

		// ���������ʧЧ���細�ڴ�С�ı䣩�������ؽ�
		if (ret == VK_ERROR_OUT_OF_DATE_KHR) {
			// �ȴ��豸������ȷ����ȫ�ؽ�
			CALL_VK(vkDeviceWaitIdle(device->GetHandle()));
			VkExtent2D originExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			bool bSuc = swapchain->ReCreate();

			VkExtent2D newExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			// ����ؽ��ɹ��ҳߴ緢���仯�������Ҫ����Ŀ��
			if (bSuc && (originExtent.width != newExtent.width || originExtent.height != newExtent.height)) {
				bShouldUpdateTarget = true;
			}

			// ���»�ȡͼ������
			ret = swapchain->AcquireImage(imageIndex, mImageAvailableSemaphores[mCurrentBuffer]);
			if (ret != VK_SUCCESS && ret != VK_SUBOPTIMAL_KHR) {
				LOG_E("Recreate swapchain error: {0}", vk_result_string(ret));
			}
		}

		// �������ֵ���������
		if (outImageIndex) {
			*outImageIndex = static_cast<int32_t>(imageIndex);
		}

		return bShouldUpdateTarget;
	}

	/**
 * @brief ������Ⱦ���̣��ύ�������������ͼ��
 *
 * �ú��������ύͼ��������������У�ִ��ͼ����ֲ�����
 * �����������ؽ������������
 *
 * @param imageIndex ��ǰҪ���ֵĽ�����ͼ������
 * @param cmdBuffers Ҫ�ύ�������������
 *
 * @return bool �Ƿ���Ҫ������ȾĿ��ߴ磬���������ؽ��ҳߴ緢���仯ʱ����true
 */
	bool AdRenderer::End(int32_t imageIndex, const std::vector<VkCommandBuffer>& cmdBuffers) {
		ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
		ade::AdVKDevice* device = renderCxt->GetDevice();
		ade::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();
		bool bShouldUpdateTarget = false;

		// �ύ���������ͼ�ζ��У����ȴ��ź���
		device->GetFirstGraphicQueue()->Submit(cmdBuffers, { mImageAvailableSemaphores[mCurrentBuffer] }, { mSubmitedSemaphores[mCurrentBuffer] }, mFrameFences[mCurrentBuffer]);

		// ִ��ͼ����ֲ���
		VkResult ret = swapchain->Present(imageIndex, { mSubmitedSemaphores[mCurrentBuffer] });

		// ������ֽ��Ϊ���ŵ��������Ҫ�ؽ�������
		if (ret == VK_SUBOPTIMAL_KHR) {
			CALL_VK(vkDeviceWaitIdle(device->GetHandle()));
			VkExtent2D originExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			bool bSuc = swapchain->ReCreate();

			VkExtent2D newExtent = { swapchain->GetWidth(), swapchain->GetHeight() };
			// ����������ؽ��ɹ��ҳߴ緢���仯�������Ҫ����Ŀ��
			if (bSuc && (originExtent.width != newExtent.width || originExtent.height != newExtent.height)) {
				bShouldUpdateTarget = true;
			}
		}

		// �ȴ��豸���в����µ�ǰ����������
		CALL_VK(vkDeviceWaitIdle(device->GetHandle()));
		mCurrentBuffer = (mCurrentBuffer + 1) % RENDERER_NUM_BUFFER;
		return bShouldUpdateTarget;
	}
}