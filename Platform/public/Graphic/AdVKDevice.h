#ifndef AD_VK_DEVICE_H
#define AD_VK_DEVICE_H

#include "AdVKCommon.h"
#include "AdVKQueue.h"

namespace WuDu {
	class AdVKGraphicContext;
	class AdVKQueue;
	class AdVKCommandPool;

	/**
	* AdVkSettings结构体用于存储Vulkan渲染设备的配置参数。
	* 这些参数在初始化Vulkan设备和渲染配置时非常重要。
	*/
	struct AdVkSettings {
		// surfaceFormat指定交换链的像素格式，默认使用B8G8R8A8_UNORM，这意味着每个通道使用8位BGR和Alpha值，以非标准化格式存储。
		VkFormat surfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;

		// depthFormat指定深度缓冲区的格式，默认使用D32_SFLOAT，表示32位浮点数深度值。
		VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

		// presentMode定义了交换链显示图像的方式，VK_PRESENT_MODE_IMMEDIATE_KHR表示图像一旦准备好就立即显示，没有延迟。
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

		// swapchainImageCount表示交换链图像的数量，默认值为3，通常使用triple buffering来平衡响应性和性能。
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