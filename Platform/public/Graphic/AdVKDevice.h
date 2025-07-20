#ifndef AD_VK_DEVICE_H
#define AD_VK_DEVICE_H

#include "AdVKCommon.h"

namespace ade {
	class AdVKGraphicContext;
	class AdVKQueue;
	class AdVKCommandPool;

	/**
	* AdVkSettings结构体用于存储Vulkan渲染相关的配置设置。
	* 这些设置对于初始化Vulkan设备、交换链以及渲染流程中非常重要。
	*/
	struct AdVkSettings {
		// surfaceFormat指定表面的像素格式，这里使用的是B8G8R8A8_UNORM，意味着每个像素使用8位BGR和Alpha值，以非规范化形式存储。
		VkFormat surfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;

		// depthFormat定义了深度缓冲区的格式，这里使用的是D32_SFLOAT，表示32位浮点数深度值。
		VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

		// presentMode决定了交换链呈现图像的方式，VK_PRESENT_MODE_IMMEDIATE_KHR表示图像一旦准备好就立即呈现，减少延迟。
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

		// swapchainImageCount表示交换链中图像的数量，这里设置为3，通常与 triple buffering（三缓冲）对应，以优化性能和减少卡顿。
		uint32_t swapchainImageCount = 3;
	};
	class AdVKDevice {
	public:
		AdVKDevice(AdVKGraphicContext* context, uint32_t graphicQueueCount, uint32_t presentQueueCount, const AdVkSettings& settings = {});
		~AdVKDevice();

		VkDevice GetHandle() const { return mHandle; }

		const AdVkSettings& GetSettings() const { return mSettings; }
		VkPipelineCache GetPipelineCache() const { return mPipelineCache; }

		AdVKQueue* GetGraphicQueue(uint32_t index) const { return mGraphicQueues.size() < index + 1 ? nullptr : mGraphicQueues[index].get(); };
		AdVKQueue* GetFirstGraphicQueue() const { return mGraphicQueues.empty() ? nullptr : mGraphicQueues[0].get(); };
		AdVKQueue* GetPresentQueue(uint32_t index) const { return mPresentQueues.size() < index + 1 ? nullptr : mPresentQueues[index].get(); };
		AdVKQueue* GetFirstPresentQueue() const { return mPresentQueues.empty() ? nullptr : mPresentQueues[0].get(); };
		AdVKCommandPool* GetDefaultCmdPool() const { return mDefaultCmdPool.get(); }

		int32_t GetMemoryIndex(VkMemoryPropertyFlags memProps, uint32_t memoryTypeBits) const;
		VkCommandBuffer CreateAndBeginOneCmdBuffer();
		void SubmitOneCmdBuffer(VkCommandBuffer cmdBuffer);

		VkResult CreateSimpleSampler(VkFilter filter, VkSamplerAddressMode addressMode, VkSampler* outSampler);
	private:
		void CreatePipelineCache();
		void CreateDefaultCmdPool();

		VkDevice mHandle = VK_NULL_HANDLE;
		AdVKGraphicContext* mContext;

		std::vector<std::shared_ptr<AdVKQueue>> mGraphicQueues;
		std::vector<std::shared_ptr<AdVKQueue>> mPresentQueues;
		std::shared_ptr<AdVKCommandPool> mDefaultCmdPool;

		AdVkSettings mSettings;

		VkPipelineCache mPipelineCache = VK_NULL_HANDLE;
	};
}

#endif