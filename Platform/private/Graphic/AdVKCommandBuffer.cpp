#include "Graphic/AdVKCommandBuffer.h"
#include "Graphic/AdVKDevice.h"

namespace ade {

	/**
	 * @brief ���캯�������ڴ��� Vulkan ����ض���
	 *
	 * ��ʼ�� Vulkan ����أ���ͨ�� vkCreateCommandPool ������Ӧ�� Vulkan ����ؾ����
	 *
	 * @param device ָ�� Vulkan �豸�����ָ�룬���ڻ�ȡ Vulkan �߼��豸���
	 * @param queueFamilyIndex ָ��������������Ķ���������
	 */
	AdVKCommandPool::AdVKCommandPool(AdVKDevice* device, uint32_t queueFamilyIndex) : mDevice(device) {
		VkCommandPoolCreateInfo commandPoolInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queueFamilyIndex
		};
		// ���� Vulkan API ���������
		CALL_VK(vkCreateCommandPool(mDevice->GetHandle(), &commandPoolInfo, nullptr, &mHandle));
		LOG_T("Create command pool : {0}", (void*)mHandle);
	}

	/**
	 * @brief ������������������ Vulkan ����ض���
	 *
	 * �ڶ�������ʱ���� vkDestroyCommandPool �����Ѵ������������Դ��
	 */
	AdVKCommandPool::~AdVKCommandPool() {
		// ���� Vulkan �������Դ
		VK_D(CommandPool, mDevice->GetHandle(), mHandle);
	}

	/**
	 * @brief ��������з������������
	 *
	 * ʹ�� vkAllocateCommandBuffers ����ָ��������������������������������б�
	 *
	 * @param count ��Ҫ������������������
	 * @return std::vector<VkCommandBuffer> �������з��������������������
	 */
	std::vector<VkCommandBuffer> AdVKCommandPool::AllocateCommandBuffer(uint32_t count) const {
		std::vector<VkCommandBuffer> cmdBuffers(count);
		VkCommandBufferAllocateInfo allocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = mHandle,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = count
		};
		// ���� Vulkan API �����������
		CALL_VK(vkAllocateCommandBuffers(mDevice->GetHandle(), &allocateInfo, cmdBuffers.data()));
		return cmdBuffers;
	}

	/**
	 * @brief ��������з���һ���������
	 *
	 * ��װ�� AllocateCommandBuffer ������������һ��������������ص�һ��Ԫ��
	 *
	 * @return VkCommandBuffer ���ط���ĵ�������������
	 */
	VkCommandBuffer AdVKCommandPool::AllocateOneCommandBuffer() const {
		std::vector<VkCommandBuffer> cmdBuffers = AllocateCommandBuffer(1);
		return cmdBuffers[0];
	}

	/**
	 * @brief ��ʼ��¼�������
	 *
	 * �������������״̬��Ȼ��ʼ��¼����û�������
	 *
	 * @param cmdBuffer Ҫ��ʼ��¼������������
	 */
	void AdVKCommandPool::BeginCommandBuffer(VkCommandBuffer cmdBuffer) {
		// �������������׼�����¼�¼
		CALL_VK(vkResetCommandBuffer(cmdBuffer, 0));
		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};
		// ��ʼ��¼�������
		CALL_VK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
	}

	/**
	 * @brief ������������ļ�¼
	 *
	 * ��������¼����ô˺�����������������Ĳ���
	 *
	 * @param cmdBuffer Ҫ������¼������������
	 */
	void AdVKCommandPool::EndCommandBuffer(VkCommandBuffer cmdBuffer) {
		// ������������ļ�¼
		CALL_VK(vkEndCommandBuffer(cmdBuffer));
	}
}