#include "Graphic/AdVKQueue.h"

namespace WuDu {
	AdVKQueue::AdVKQueue(uint32_t familyIndex, uint32_t index, VkQueue queue, bool canPresent)
		: mFamilyIndex(familyIndex), mIndex(index), mHandle(queue), canPresent(canPresent) {
		LOG_T("Create a new queue: {0} - {1} - {2}, present: {3}", mFamilyIndex, index, (void*)queue, canPresent);
	}

	void AdVKQueue::WaitIdle() const {
		CALL_VK(vkQueueWaitIdle(mHandle));
	}

		/**
		* 提交命令缓冲区到VK队列中执行
		*
		* 此函数将一系列命令缓冲区提交到VK队列中执行，同时可以指定等待和信号量来同步执行过程
		* 它主要用于图形或计算任务的执行，是VK编程中不可或缺的一部分
		*
		* @param cmdBuffers 包含要提交执行的命令缓冲区的向量
		* @param waitSemaphores 在执行命令缓冲区之前需要等待的信号量向量
		* @param signalSemaphores 命令缓冲区执行完成后要触发的信号量向量
		* @param frameFence 命令缓冲区执行完成后要触发的栅栏，用于CPU和GPU的同步
		*/
	void AdVKQueue::Submit(const std::vector<VkCommandBuffer>& cmdBuffers, const std::vector<VkSemaphore>& waitSemaphores, const std::vector<VkSemaphore>& signalSemaphores, VkFence frameFence) {
		// 指定在绘制命令执行的哪个阶段等待信号量
		VkPipelineStageFlags waitDstStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		// 初始化提交信息结构体
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

		// 调用VK函数提交命令缓冲区到队列
		CALL_VK(vkQueueSubmit(mHandle, 1, &submitInfo, frameFence));
	}
}