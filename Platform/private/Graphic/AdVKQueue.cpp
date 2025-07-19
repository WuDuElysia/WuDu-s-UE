#include "Graphic/AdVKQueue.h"

namespace ade {
	AdVKQueue::AdVKQueue(uint32_t familyIndex, uint32_t index, VkQueue queue, bool canPresent)
		: mFamilyIndex(familyIndex), mIndex(index), mHandle(queue), canPresent(canPresent) {
		LOG_T("Create a new queue: {0} - {1} - {2}, present: {3}", mFamilyIndex, index, (void*)queue, canPresent);
	}

	void AdVKQueue::WaitIdle() const {
		CALL_VK(vkQueueWaitIdle(mHandle));
	}

		/**
		* �ύ���������VK������ִ��
		*
		* �˺�����һϵ����������ύ��VK������ִ�У�ͬʱ����ָ���ȴ����ź�����ͬ��ִ�й���
		* ����Ҫ����ͼ�λ���������ִ�У���VK����в��ɻ�ȱ��һ����
		*
		* @param cmdBuffers ����Ҫ�ύִ�е��������������
		* @param waitSemaphores ��ִ���������֮ǰ��Ҫ�ȴ����ź�������
		* @param signalSemaphores �������ִ����ɺ�Ҫ�������ź�������
		* @param frameFence �������ִ����ɺ�Ҫ������դ��������CPU��GPU��ͬ��
		*/
	void AdVKQueue::Submit(const std::vector<VkCommandBuffer>& cmdBuffers, const std::vector<VkSemaphore>& waitSemaphores, const std::vector<VkSemaphore>& signalSemaphores, VkFence frameFence) {
		// ָ���ڻ�������ִ�е��ĸ��׶εȴ��ź���
		VkPipelineStageFlags waitDstStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		// ��ʼ���ύ��Ϣ�ṹ��
		VkSubmitInfo submitInfo = {
		    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		    submitInfo.pNext = nullptr,
		    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
		    submitInfo.pWaitSemaphores = waitSemaphores.data(),
		    submitInfo.pWaitDstStageMask = waitDstStageMask,
		    submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size()),
		    submitInfo.pCommandBuffers = cmdBuffers.data(),
		    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
		    submitInfo.pSignalSemaphores = signalSemaphores.data()
		};

		// ����VK�����ύ�������������
		CALL_VK(vkQueueSubmit(mHandle, 1, &submitInfo, frameFence));
	}
}