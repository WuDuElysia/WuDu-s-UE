#include "Graphic/AdVKDevice.h"
#include "Graphic/AdVKGraphicContext.h"
#include "Graphic/AdVKQueue.h"
//#include "Graphic/AdVKCommandBuffer.h"

namespace ade {
	const DeviceFeature requestedExtensions[] = {
		{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, true },
    #ifdef AD_ENGINE_PLATFORM_WIN32
    #elif AD_ENGINE_PLATFORM_MACOS
		{ "VK_KHR_portability_subset", true },
    #elif AD_ENGINE_PLATFORM_LINUX
    #endif
	};

	// ���캯��: ��ʼ��AdVKDevice����
	// ����:
	// - context: ָ��AdVKGraphicContext�����ָ�룬���ڻ�ȡͼ�κ���ʾ���м�����Ϣ
	// - graphicQueueCount: ͼ�ζ��е�����
	// - presentQueueCount: ��ʾ���е�����
	// - settings: Vulkan���ã������豸����
	AdVKDevice::AdVKDevice(AdVKGraphicContext* context, uint32_t graphicQueueCount, uint32_t presentQueueCount, const AdVkSettings& settings) : mContext(context), mSettings(settings) {
		// ���context�Ƿ�Ϊnullptr
		if (!context) {
			LOG_E("Must create a vulkan graphic context before create device.");
			return;
		}

		// ��ȡͼ�κ���ʾ���м�����Ϣ
		QueueFamilyInfo graphicQueueFamilyInfo = context->GetGraphicQueueFamilyInfo();
		QueueFamilyInfo presentQueueFamilyInfo = context->GetPresentQueueFamilyInfo();

		// �������Ķ��������Ƿ񳬹����м����ʵ������
		if (graphicQueueCount > graphicQueueFamilyInfo.queueCount) {
			LOG_E("this queue family has {0} queue, but request {1}", graphicQueueFamilyInfo.queueCount, graphicQueueCount);
			return;
		}
		if (presentQueueCount > presentQueueFamilyInfo.queueCount) {
			LOG_E("this queue family has {0} queue, but request {1}", presentQueueFamilyInfo.queueCount, presentQueueCount);
			return;
		}

		// ��ʼ��ͼ�κ���ʾ���е����ȼ�
		std::vector<float> graphicQueuePriorities(graphicQueueCount, 0.f);
		std::vector<float> presentQueuePriorities(graphicQueueCount, 1.f);

		// ���ͼ�κ���ʾ���м��������Ƿ���ͬ
		bool bSameQueueFamilyIndex = context->IsSameGraphicPresentQueueFamily();
		uint32_t sameQueueCount = graphicQueueCount;
		if (bSameQueueFamilyIndex) {
			sameQueueCount += presentQueueCount;
			if (sameQueueCount > graphicQueueFamilyInfo.queueCount) {
				sameQueueCount = graphicQueueFamilyInfo.queueCount;
			}
			graphicQueuePriorities.insert(graphicQueuePriorities.end(), presentQueuePriorities.begin(), presentQueuePriorities.end());
		}

		// ׼�����д�����Ϣ
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

		// ���ͼ�κ���ʾ���м���������ͬ����ӵڶ������д�����Ϣ
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

		// ö���豸��չ����
		uint32_t availableExtensionCount;
		CALL_VK(vkEnumerateDeviceExtensionProperties(context->GetPhyDevice(), "", &availableExtensionCount, nullptr));
		std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
		CALL_VK(vkEnumerateDeviceExtensionProperties(context->GetPhyDevice(), "", &availableExtensionCount, availableExtensions.data()));

		// ��鲢�����豸��չ
		uint32_t enableExtensionCount;
		const char* enableExtensions[32];
		if (!checkDeviceFeatures("Device Extension", true, availableExtensionCount, availableExtensions.data(),
			ARRAY_SIZE(requestedExtensions), requestedExtensions, &enableExtensionCount, enableExtensions)) {
			return;
		}

		// ׼���豸������Ϣ
		VkDeviceCreateInfo deviceInfo = {
		    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		    deviceInfo.pNext = nullptr,
		    deviceInfo.flags = 0,
		    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(bSameQueueFamilyIndex ? 1 : 2),
		    deviceInfo.pQueueCreateInfos = queueInfos,
		    deviceInfo.enabledLayerCount = 0,
		    deviceInfo.ppEnabledLayerNames = nullptr,
		    deviceInfo.enabledExtensionCount = enableExtensionCount,
		    deviceInfo.ppEnabledExtensionNames = enableExtensionCount > 0 ? enableExtensions : nullptr,
		    deviceInfo.pEnabledFeatures = nullptr
		};

		// �����߼��豸
		CALL_VK(vkCreateDevice(context->GetPhyDevice(), &deviceInfo, nullptr, &mHandle));
		LOG_T("VkDevice: {0}", (void*)mHandle);

		// ��ȡ������ͼ�ζ���
		for (int i = 0; i < graphicQueueCount; i++) {
			VkQueue queue;
			vkGetDeviceQueue(mHandle, graphicQueueFamilyInfo.queueFamilyIndex, i, &queue);
			mGraphicQueues.push_back(std::make_shared<AdVKQueue>(graphicQueueFamilyInfo.queueFamilyIndex, i, queue, false));
		}

		// ��ȡ��������ʾ����
		for (int i = 0; i < presentQueueCount; i++) {
			VkQueue queue;
			vkGetDeviceQueue(mHandle, presentQueueFamilyInfo.queueFamilyIndex, i, &queue);
			mPresentQueues.push_back(std::make_shared<AdVKQueue>(presentQueueFamilyInfo.queueFamilyIndex, i, queue, true));
		}

		// �����ܵ�����
		CreatePipelineCache();

		// ����Ĭ�������
		CreateDefaultCmdPool();
	}

	// AdVKDevice�����������
	AdVKDevice::~AdVKDevice() {
		// �ȴ��豸���У�ȷ�������������ִ��
		vkDeviceWaitIdle(mHandle);
		// �ͷ�Ĭ�������
		mDefaultCmdPool = nullptr;
		// ���ٹܵ�����
		VK_D(PipelineCache, mHandle, mPipelineCache);
		// �����豸
		vkDestroyDevice(mHandle, nullptr);
	}

	//// �����ܵ�����
	//void AdVKDevice::CreatePipelineCache() {
	//	// ��ʼ���ܵ����洴����Ϣ
	//	VkPipelineCacheCreateInfo pipelineCacheInfo = {
	//		pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
	//		pipelineCacheInfo.pNext = nullptr,
	//		pipelineCacheInfo.flags = 0
	//	};
	//	// ����vkCreatePipelineCache�����ܵ�����
	//	CALL_VK(vkCreatePipelineCache(mHandle, &pipelineCacheInfo, nullptr, &mPipelineCache));
	//}

	//// ����Ĭ�������
	//void AdVKDevice::CreateDefaultCmdPool() {
	//	// ʹ�� Adele��� AdVKCommandPool�ഴ��Ĭ�������
	//	mDefaultCmdPool = std::make_shared<ade::AdVKCommandPool>(this, mContext->GetGraphicQueueFamilyInfo().queueFamilyIndex);
	//}

	//// ��ȡ�ڴ���������
	//int32_t AdVKDevice::GetMemoryIndex(VkMemoryPropertyFlags memProps, uint32_t memoryTypeBits) const {
	//	// ��ȡ�����豸�ڴ�����
	//	VkPhysicalDeviceMemoryProperties phyDeviceMemProps = mContext->GetPhyDeviceMemProperties();
	//	// ����ڴ����������Ƿ�Ϊ0
	//	if (phyDeviceMemProps.memoryTypeCount == 0) {
	//		LOG_E("Physical device memory type count is 0");
	//		return -1;
	//	}
	//	// ���������ڴ����ͣ�Ѱ��ƥ����ڴ���������
	//	for (int i = 0; i < phyDeviceMemProps.memoryTypeCount; i++) {
	//		if (memoryTypeBits & (1 << i) && (phyDeviceMemProps.memoryTypes[i].propertyFlags & memProps) == memProps) {
	//			return i;
	//		}
	//	}
	//	// ����Ҳ���ƥ����ڴ����ͣ���¼���󲢷���0
	//	LOG_E("Can not find memory type index: type bit: {0}", memoryTypeBits);
	//	return 0;
	//}

	//VkCommandBuffer AdVKDevice::CreateAndBeginOneCmdBuffer() {
	//        VkCommandBuffer cmdBuffer = mDefaultCmdPool->AllocateOneCommandBuffer();
	//        mDefaultCmdPool->BeginCommandBuffer(cmdBuffer);
	//        return cmdBuffer;
	//}

	//void AdVKDevice::SubmitOneCmdBuffer(VkCommandBuffer cmdBuffer) {
	//        mDefaultCmdPool->EndCommandBuffer(cmdBuffer);
	//        AdVKQueue* queue = GetFirstGraphicQueue();
	//        queue->Submit({ cmdBuffer });
	//        queue->WaitIdle();
	//}

	///**
	// * ����һ���򵥵Ĳ���������
	// *
	// * �ú�������ָ�����˲����͵�ַģʽ����һ��VkSampler�����������������
	// * �������˲������ĳ������ԣ��������˲�����ַģʽ�ȣ���ͨ�� Vulkan ��
	// * vkCreateSampler ������������������
	// *
	// * @param filter ָ������magFilter��minFilter���˲������ͣ������������ʱ���˲���ʽ��
	// * @param addressMode ָ���������곬��[0, 1]��Χʱ�Ĵ���ʽ��
	// * @param outSampler ָ��һ��VkSampler�����ָ�룬���ڽ��մ����Ĳ���������
	// *
	// * @return ����VkResultֵ��ָʾ���������Ľ����VK_SUCCESS��ʾ�ɹ�������ֵ��ʾ��ͬ�Ĵ��������
	// */
	//VkResult AdVKDevice::CreateSimpleSampler(VkFilter filter, VkSamplerAddressMode addressMode, VkSampler* outSampler) {
	//	// ��ʼ��VkSamplerCreateInfo�ṹ�壬���ò����������ԡ�
	//	VkSamplerCreateInfo samplerInfo = {
	//	    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, // ָ���ṹ������
	//	    samplerInfo.pNext = nullptr, // ָ����һ���ṹ���ָ�룬ͨ��Ϊnullptr
	//	    samplerInfo.flags = 0, // �����ֶΣ�����δ��ʹ�ã�ĿǰӦ����Ϊ0
	//	    samplerInfo.magFilter = filter, // ָ���Ŵ��˲���
	//	    samplerInfo.minFilter = filter, // ָ����С�˲���
	//	    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR, // ָ�� mip ӳ����˲���ʽ
	//	    samplerInfo.addressModeU = addressMode, // ָ�� U ��ĵ�ַģʽ
	//	    samplerInfo.addressModeV = addressMode, // ָ�� V ��ĵ�ַģʽ
	//	    samplerInfo.addressModeW = addressMode, // ָ�� W ��ĵ�ַģʽ
	//	    samplerInfo.mipLodBias = 0, // mip LOD ƫ���������Ϊ0
	//	    samplerInfo.anisotropyEnable = VK_FALSE, // �Ƿ����ø������Թ��ˣ��������
	//	    samplerInfo.maxAnisotropy = 0, // ���������Գ̶ȣ�������һ����ã���������Ϊ0
	//	    samplerInfo.compareEnable = VK_FALSE, // �Ƿ�������ȱȽϣ��������
	//	    samplerInfo.compareOp = VK_COMPARE_OP_NEVER, // ��ȱȽϲ�����������һ����ã���������Ϊ�Ӳ��Ƚ�
	//	    samplerInfo.minLod = 0, // �������СLOD����
	//	    samplerInfo.maxLod = 1, // ��������LOD����
	//	    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK, // �߿���ɫ�����ڵ�ַģʽΪ�߿�ʱ
	//	    samplerInfo.unnormalizedCoordinates = VK_FALSE // �Ƿ�ʹ�÷Ǳ�׼�����꣬�������
	//	};

	//	// ����Vulkan��vkCreateSampler����������������Ϣ��������������
	//	return vkCreateSampler(mHandle, &samplerInfo, nullptr, outSampler);
	//}
}