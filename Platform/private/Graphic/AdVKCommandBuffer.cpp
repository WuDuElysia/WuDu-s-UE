#include "Graphic/AdVKCommandBuffer.h"
#include "Graphic/AdVKDevice.h"

namespace WuDu {

	/**
	 * @brief 构造函数，用于创建 Vulkan 命令池对象
	 *
	 * 初始化 Vulkan 命令池，并通过 vkCreateCommandPool 创建对应的 Vulkan 命令池句柄。
	 *
	 * @param device 指向 Vulkan 设备对象的指针，用于获取 Vulkan 逻辑设备句柄
	 * @param queueFamilyIndex 指定命令池所关联的队列族索引
	 */
	AdVKCommandPool::AdVKCommandPool(AdVKDevice* device, uint32_t queueFamilyIndex) : mDevice(device) {
		VkCommandPoolCreateInfo commandPoolInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queueFamilyIndex
		};
		// 调用 Vulkan API 创建命令池
		CALL_VK(vkCreateCommandPool(mDevice->GetHandle(), &commandPoolInfo, nullptr, &mHandle));
		LOG_T("Create command pool : {0}", (void*)mHandle);
	}

	/**
	 * @brief 析构函数，用于销毁 Vulkan 命令池对象
	 *
	 * 在对象销毁时调用 vkDestroyCommandPool 销毁已创建的命令池资源。
	 */
	AdVKCommandPool::~AdVKCommandPool() {
		// 销毁 Vulkan 命令池资源
		VK_D(CommandPool, mDevice->GetHandle(), mHandle);
	}

	/**
	 * @brief 从命令池中分配多个命令缓冲区
	 *
	 * 使用 vkAllocateCommandBuffers 分配指定数量的主级命令缓冲区，并返回其句柄列表
	 *
	 * @param count 需要分配的命令缓冲区的数量
	 * @return std::vector<VkCommandBuffer> 包含所有分配的命令缓冲区句柄的向量
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
		// 调用 Vulkan API 分配命令缓冲区
		CALL_VK(vkAllocateCommandBuffers(mDevice->GetHandle(), &allocateInfo, cmdBuffers.data()));
		return cmdBuffers;
	}

	/**
	 * @brief 从命令池中分配一个命令缓冲区
	 *
	 * 封装了 AllocateCommandBuffer 方法，仅分配一个命令缓冲区并返回第一个元素
	 *
	 * @return VkCommandBuffer 返回分配的单个命令缓冲区句柄
	 */
	VkCommandBuffer AdVKCommandPool::AllocateOneCommandBuffer() const {
		std::vector<VkCommandBuffer> cmdBuffers = AllocateCommandBuffer(1);
		return cmdBuffers[0];
	}

	/**
	 * @brief 开始记录命令缓冲区
	 *
	 * 先重置命令缓冲区状态，然后开始记录命令到该缓冲区中
	 *
	 * @param cmdBuffer 要开始记录的命令缓冲区句柄
	 */
	void AdVKCommandPool::BeginCommandBuffer(VkCommandBuffer cmdBuffer) {
		// 重置命令缓冲区以准备重新记录
		CALL_VK(vkResetCommandBuffer(cmdBuffer, 0));
		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};
		// 开始记录命令缓冲区
		CALL_VK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
	}

	/**
	 * @brief 结束命令缓冲区的记录
	 *
	 * 完成命令记录后调用此函数结束对命令缓冲区的操作
	 *
	 * @param cmdBuffer 要结束记录的命令缓冲区句柄
	 */
	void AdVKCommandPool::EndCommandBuffer(VkCommandBuffer cmdBuffer) {
		// 结束命令缓冲区的记录
		CALL_VK(vkEndCommandBuffer(cmdBuffer));
	}
}