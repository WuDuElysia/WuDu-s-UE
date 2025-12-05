#include "Graphic/AdVKDevice.h"
#include "Graphic/AdVKGraphicContext.h"
#include "Graphic/AdVKQueue.h"
#include "Graphic/AdVKCommandBuffer.h"

namespace WuDu {
	const DeviceFeature requestedExtensions[] = {
		{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, true },
    #ifdef AD_ENGINE_PLATFORM_WIN32
    #elif AD_ENGINE_PLATFORM_MACOS
		{ "VK_KHR_portability_subset", true },
    #elif AD_ENGINE_PLATFORM_LINUX
    #endif
	};

	// 构造函数: 初始化AdVKDevice对象
	// 参数:
	// - context: 指向AdVKGraphicContext对象的指针，用于获取图形和显示队列家族信息
	// - graphicQueueCount: 图形队列的数量
	// - presentQueueCount: 显示队列的数量
	// - settings: Vulkan设置，用于设备配置
	AdVKDevice::AdVKDevice(AdVKGraphicContext* context, uint32_t graphicQueueCount, uint32_t presentQueueCount, const AdVkSettings& settings) : mContext(context), mSettings(settings) {
		// 检查context是否为nullptr
		if (!context) {
			LOG_E("Must create a vulkan graphic context before create device.");
			return;
		}

		// 获取图形和显示队列家族信息
		QueueFamilyInfo graphicQueueFamilyInfo = context->GetGraphicQueueFamilyInfo();
		QueueFamilyInfo presentQueueFamilyInfo = context->GetPresentQueueFamilyInfo();

		// 检查请求的队列数量是否超过队列家族的实际数量
		if (graphicQueueCount > graphicQueueFamilyInfo.queueCount) {
			LOG_E("this queue family has {0} queue, but request {1}", graphicQueueFamilyInfo.queueCount, graphicQueueCount);
			return;
		}
		if (presentQueueCount > presentQueueFamilyInfo.queueCount) {
			LOG_E("this queue family has {0} queue, but request {1}", presentQueueFamilyInfo.queueCount, presentQueueCount);
			return;
		}

		// 初始化图形和显示队列的优先级
		std::vector<float> graphicQueuePriorities(graphicQueueCount, 0.f);
		std::vector<float> presentQueuePriorities(graphicQueueCount, 1.f);

		// 检查图形和显示队列家族索引是否相同
		bool bSameQueueFamilyIndex = context->IsSameGraphicPresentQueueFamily();
		uint32_t sameQueueCount = graphicQueueCount;
		if (bSameQueueFamilyIndex) {
			sameQueueCount += presentQueueCount;
			if (sameQueueCount > graphicQueueFamilyInfo.queueCount) {
				sameQueueCount = graphicQueueFamilyInfo.queueCount;
			}
			graphicQueuePriorities.insert(graphicQueuePriorities.end(), presentQueuePriorities.begin(), presentQueuePriorities.end());
		}

		// 准备队列创建信息
		VkDeviceQueueCreateInfo queueInfos[2] = {
		    {
			//VkStructureType             sType;
			// const void* pNext;
			 // VkDeviceQueueCreateFlags    flags;
			 // uint32_t                    queueFamilyIndex;
			 // uint32_t                    queueCount;
			// const float* pQueuePriorities;
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(graphicQueueFamilyInfo.queueFamilyIndex),
			sameQueueCount,
			graphicQueuePriorities.data()
		    }
		};

		// 如果图形和显示队列家族索引不同，添加第二个队列创建信息
		if (!bSameQueueFamilyIndex) {
			queueInfos[1] = {
			    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			    nullptr,
			    0,
			    static_cast<uint32_t>(presentQueueFamilyInfo.queueFamilyIndex),
			    presentQueueCount,
			    presentQueuePriorities.data()
			};
		}

		// 枚举设备扩展属性
		uint32_t availableExtensionCount;
		CALL_VK(vkEnumerateDeviceExtensionProperties(context->GetPhyDevice(), "", &availableExtensionCount, nullptr));
		std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
		CALL_VK(vkEnumerateDeviceExtensionProperties(context->GetPhyDevice(), "", &availableExtensionCount, availableExtensions.data()));


		// 检查并启用设备扩展
		uint32_t enableExtensionCount;
		std::vector<const char*> enableExtensions(32);
		enableExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		enableExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
		if (!checkDeviceFeatures("Device Extension", true, availableExtensionCount, availableExtensions.data(),
			ARRAY_SIZE(requestedExtensions), requestedExtensions, &enableExtensionCount, enableExtensions.data())) {
			return;
		}

		// 2. 启用动态渲染特性（通过pNext链）
		VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {};
		dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		dynamicRenderingFeatures.dynamicRendering = VK_TRUE; // 关键：启用特性

		// 准备设备创建信息
		VkDeviceCreateInfo deviceInfo = {
		    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		    deviceInfo.pNext = &dynamicRenderingFeatures,
		    deviceInfo.flags = 0,
		    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(bSameQueueFamilyIndex ? 1 : 2),
		    deviceInfo.pQueueCreateInfos = queueInfos,
		    deviceInfo.enabledLayerCount = 0,
		    deviceInfo.ppEnabledLayerNames = nullptr,
		    deviceInfo.enabledExtensionCount = enableExtensionCount,
		    deviceInfo.ppEnabledExtensionNames = enableExtensionCount > 0 ? enableExtensions.data() : nullptr,
		    deviceInfo.pEnabledFeatures = nullptr
		};

		// 创建逻辑设备
		CALL_VK(vkCreateDevice(context->GetPhyDevice(), &deviceInfo, nullptr, &mHandle));
		LOG_T("VkDevice: {0}", (void*)mHandle);

		// 获取并保存图形队列
		for (int i = 0; i < graphicQueueCount; i++) {
			VkQueue queue;
			vkGetDeviceQueue(mHandle, graphicQueueFamilyInfo.queueFamilyIndex, i, &queue);
			mGraphicQueues.push_back(std::make_shared<AdVKQueue>(graphicQueueFamilyInfo.queueFamilyIndex, i, queue, false));
		}

		// 获取并保存显示队列
		for (int i = 0; i < presentQueueCount; i++) {
			VkQueue queue;
			vkGetDeviceQueue(mHandle, presentQueueFamilyInfo.queueFamilyIndex, i, &queue);
			mPresentQueues.push_back(std::make_shared<AdVKQueue>(presentQueueFamilyInfo.queueFamilyIndex, i, queue, true));
		}

		// 创建管道缓存
		CreatePipelineCache();

		// 创建默认命令池
		CreateDefaultCmdPool();
	}

	// AdVKDevice类的析构函数
	AdVKDevice::~AdVKDevice() {
		// 等待设备空闲，确保所有命令完成执行
		vkDeviceWaitIdle(mHandle);
		// 释放默认命令池
		mDefaultCmdPool = nullptr;
		// 销毁管道缓存
		VK_D(PipelineCache, mHandle, mPipelineCache);
		// 销毁设备
		vkDestroyDevice(mHandle, nullptr);
	}

	// 创建管道缓存
	void AdVKDevice::CreatePipelineCache() {
		// 初始化管道缓存创建信息
		VkPipelineCacheCreateInfo pipelineCacheInfo = {
			pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
			pipelineCacheInfo.pNext = nullptr,
			pipelineCacheInfo.flags = 0
		};
		// 调用vkCreatePipelineCache创建管道缓存
		CALL_VK(vkCreatePipelineCache(mHandle, &pipelineCacheInfo, nullptr, &mPipelineCache));
	}

	// 创建默认命令池
	void AdVKDevice::CreateDefaultCmdPool() {
		// 使用 Adele库的 AdVKCommandPool类创建默认命令池
		mDefaultCmdPool = std::make_shared<WuDu::AdVKCommandPool>(this, mContext->GetGraphicQueueFamilyInfo().queueFamilyIndex);
	}

	// 获取内存类型索引
	int32_t AdVKDevice::GetMemoryIndex(VkMemoryPropertyFlags memProps, uint32_t memoryTypeBits) const {
		// 获取物理设备内存属性
		VkPhysicalDeviceMemoryProperties phyDeviceMemProps = mContext->GetPhyDeviceMemProperties();
		// 检查内存类型数量是否为0
		if (phyDeviceMemProps.memoryTypeCount == 0) {
			LOG_E("Physical device memory type count is 0");
			return -1;
		}
		// 遍历所有内存类型，寻找匹配的内存类型索引
		for (int i = 0; i < phyDeviceMemProps.memoryTypeCount; i++) {
			if (memoryTypeBits & (1 << i) && (phyDeviceMemProps.memoryTypes[i].propertyFlags & memProps) == memProps) {
				return i;
			}
		}
		// 如果找不到匹配的内存类型，记录错误并返回0
		LOG_E("Can not find memory type index: type bit: {0}", memoryTypeBits);
		return 0;
	}

	VkCommandBuffer AdVKDevice::CreateAndBeginOneCmdBuffer() {
	        VkCommandBuffer cmdBuffer = mDefaultCmdPool->AllocateOneCommandBuffer();
	        mDefaultCmdPool->BeginCommandBuffer(cmdBuffer);
	        return cmdBuffer;
	}

	void AdVKDevice::SubmitOneCmdBuffer(VkCommandBuffer cmdBuffer) {
	        mDefaultCmdPool->EndCommandBuffer(cmdBuffer);
	        AdVKQueue* queue = GetFirstGraphicQueue();
	        queue->Submit({ cmdBuffer });
	        queue->WaitIdle();
	}

	/**
	 * 创建一个简单的采样器对象。
	 *
	 * 该函数根据指定的滤波器和地址模式创建一个VkSampler对象，用于纹理采样。
	 * 它配置了采样器的常见属性，如纹理滤波、地址模式等，并通过 Vulkan 的
	 * vkCreateSampler 函数创建采样器对象。
	 *
	 * @param filter 指定用于magFilter和minFilter的滤波器类型，控制纹理采样时的滤波方式。
	 * @param addressMode 指定纹理坐标超出[0, 1]范围时的处理方式。
	 * @param outSampler 指向一个VkSampler对象的指针，用于接收创建的采样器对象。
	 *
	 * @return 返回VkResult值，指示创建操作的结果。VK_SUCCESS表示成功，其他值表示不同的错误情况。
	 */
	VkResult AdVKDevice::CreateSimpleSampler(VkFilter filter, VkSamplerAddressMode addressMode, VkSampler* outSampler) {
		// 初始化VkSamplerCreateInfo结构体，设置采样器的属性。
		VkSamplerCreateInfo samplerInfo = {
		    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, // 指定结构体类型
		    samplerInfo.pNext = nullptr, // 指向下一个结构体的指针，通常为nullptr
		    samplerInfo.flags = 0, // 保留字段，用于未来使用，目前应设置为0
		    samplerInfo.magFilter = filter, // 指定放大滤波器
		    samplerInfo.minFilter = filter, // 指定缩小滤波器
		    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR, // 指定 mip 映射的滤波方式
		    samplerInfo.addressModeU = addressMode, // 指定 U 轴的地址模式
		    samplerInfo.addressModeV = addressMode, // 指定 V 轴的地址模式
		    samplerInfo.addressModeW = addressMode, // 指定 W 轴的地址模式
		    samplerInfo.mipLodBias = 0, // mip LOD 偏差，这里设置为0
		    samplerInfo.anisotropyEnable = VK_FALSE, // 是否启用各向异性过滤，这里禁用
		    samplerInfo.maxAnisotropy = 0, // 最大各向异性程度，由于上一项禁用，这里设置为0
		    samplerInfo.compareEnable = VK_FALSE, // 是否启用深度比较，这里禁用
		    samplerInfo.compareOp = VK_COMPARE_OP_NEVER, // 深度比较操作，由于上一项禁用，这里设置为从不比较
		    samplerInfo.minLod = 0, // 允许的最小LOD级别
		    samplerInfo.maxLod = 1, // 允许的最大LOD级别
		    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK, // 边框颜色，用于地址模式为边框时
		    samplerInfo.unnormalizedCoordinates = VK_FALSE // 是否使用非标准化坐标，这里禁用
		};

		// 调用Vulkan的vkCreateSampler函数，根据配置信息创建采样器对象。
		return vkCreateSampler(mHandle, &samplerInfo, nullptr, outSampler);
	}
}