#ifndef AD_VK_DEVICE_H
#define AD_VK_DEVICE_H

#include "AdVKCommon.h"

namespace ade {
	class AdVKGraphicContext;
	class AdVKQueue;
	class AdVKCommandPool;

	/**
	* AdVkSettings�ṹ�����ڴ洢Vulkan��Ⱦ��ص��������á�
	* ��Щ���ö��ڳ�ʼ��Vulkan�豸���������Լ���Ⱦ�����зǳ���Ҫ��
	*/
	struct AdVkSettings {
		// surfaceFormatָ����������ظ�ʽ������ʹ�õ���B8G8R8A8_UNORM����ζ��ÿ������ʹ��8λBGR��Alphaֵ���Էǹ淶����ʽ�洢��
		VkFormat surfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;

		// depthFormat��������Ȼ������ĸ�ʽ������ʹ�õ���D32_SFLOAT����ʾ32λ���������ֵ��
		VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

		// presentMode�����˽���������ͼ��ķ�ʽ��VK_PRESENT_MODE_IMMEDIATE_KHR��ʾͼ��һ��׼���þ��������֣������ӳ١�
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

		// swapchainImageCount��ʾ��������ͼ�����������������Ϊ3��ͨ���� triple buffering�������壩��Ӧ�����Ż����ܺͼ��ٿ��١�
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