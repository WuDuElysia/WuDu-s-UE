#include"Graphic/AdVKGraphicContext.h"
#include "Window/AdGlfwWindow.h"
#include "Window/AdGlfwWindow.h"

namespace ade {

	const DeviceFeature requesetLayers[] = {
		{ "VK_LAYER_KHRONOS_validation", true },
	};

	const DeviceFeature requesetExtensions[] = {
		{ VK_KHR_SURFACE_EXTENSION_NAME, true },
#ifdef AD_ENABLE_PLATFORM_WIN32
		{ VK_KHR_WIN32_SURFACE_EXTENSION_NAME, true },
#endif
	};

	static VkBool32 VkDebugReportCallback(VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT                  objectType,
		uint64_t                                    object,
		size_t                                      location,
		int32_t                                     messageCode,
		const char* pLayerPrefix,
		const char* pMessage,
		void* pUserData) {
		if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
			LOG_E("{0}", pMessage);
		}
		if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT || flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
			LOG_W("{0}", pMessage);
		}
		return VK_TRUE;
	}
	AdVKGraphicContext::AdVKGraphicContext(AdWindow* window){
		CreateInstance();
		CreateSurface(window);
		SelectPhyDevice();
	}

	AdVKGraphicContext::~AdVKGraphicContext() {
		vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
		vkDestroyInstance(mInstance, nullptr);

	}


	// 创建实例
	void AdVKGraphicContext::CreateInstance() {
		// 查询可用的层属性数量
		uint32_t availableLayerCount;
		CALL_VK(vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr));

		// 根据可用层属性数量创建一个向量来存储这些属性
		std::vector<VkLayerProperties> availableLayers(availableLayerCount);

		// 再次调用函数，这次传入向量的指针，以获取所有可用层的属性信息
		CALL_VK(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data()));
		// 定义用于启用的层计数
		uint32_t enableLayerCount;
		// 定义用于启用的层名称数组，最多支持32个层
		const char* enableLayerNames[32];

		// 检查设备特性是否满足要求
		// 参数包括:
		// - 特性名称
		// - 是否严格检查
		// - 可用层的数量
		// - 可用层的数据`
		// - 请求的层数组大小
		// - 请求的层数组
		// - 启用层计数的指针
		// - 启用层名称的数组
		// 如果检查通过，则更新启用层计数和名称，并返回true，否则返回false
		if (bShouldValidate) {
			if (!checkDeviceFeatures("Instance Layers", false, availableLayerCount, availableLayers.data(),
				ARRAY_SIZE(requesetLayers), requesetLayers,
				&enableLayerCount, enableLayerNames)) {

				// 如果设备特性满足要求，则返回，继续后续操作
				return;
			}
		}

		// 2. 构建extension
		uint32_t availableExtensionCount;
		// 获取可用的实例扩展属性数量
		CALL_VK(vkEnumerateInstanceExtensionProperties("", &availableExtensionCount, nullptr));
		// 根据可用扩展数量创建存储扩展属性的容器
		std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
		// 获取所有可用的实例扩展属性
		CALL_VK(vkEnumerateInstanceExtensionProperties("", &availableExtensionCount, availableExtensions.data()));

		uint32_t glfwRequestedExtensionCount;
		// 获取GLFW请求的扩展名列表
		const char** glfwRequestedExtensions = glfwGetRequiredInstanceExtensions(&glfwRequestedExtensionCount);
		// 存储所有请求的扩展名，用于去重
		std::unordered_set<std::string> allRequestedExtensionSet;
		// 存储所有请求的扩展信息
		std::vector<DeviceFeature> allRequestedExtensions;
                
		// 遍历请求的扩展，去重后添加到allRequestedExtensions中
		for (const auto& item : requesetExtensions) {
			if (allRequestedExtensionSet.find(item.name) == allRequestedExtensionSet.end()) {
				allRequestedExtensionSet.insert(item.name);
				allRequestedExtensions.push_back(item);
			}
		}
		
		allRequestedExtensions.push_back({ VK_EXT_DEBUG_REPORT_EXTENSION_NAME, true });
		
		
		allRequestedExtensions.push_back({ VK_KHR_SURFACE_EXTENSION_NAME, true });
		allRequestedExtensions.push_back({ VK_KHR_WIN32_SURFACE_EXTENSION_NAME, true });
		
		// 遍历GLFW请求的扩展，去重后添加到allRequestedExtensions中
		for (int i = 0; i < glfwRequestedExtensionCount; i++) {
			const char* extensionName = glfwRequestedExtensions[i];
			if (allRequestedExtensionSet.find(extensionName) == allRequestedExtensionSet.end()) {
				allRequestedExtensionSet.insert(extensionName);
				allRequestedExtensions.push_back({ extensionName, true });
			}
		}

		uint32_t enableExtensionCount;
		const char* enableExtensions[32];
		// 检查设备特性，确定启用的扩展列表
		if (!checkDeviceFeatures("Instance Extension", true, availableExtensionCount, availableExtensions.data(),
			allRequestedExtensions.size(), allRequestedExtensions.data(), &enableExtensionCount, enableExtensions)) {
			return;
		}
		
		// 填充应用程序信息结构体
		VkApplicationInfo applicationInfo = {
				VK_STRUCTURE_TYPE_APPLICATION_INFO,
				nullptr,
				"WuDu_Engine",
				VK_MAKE_VERSION(1, 0, 0),
				"None",
				VK_MAKE_VERSION(1, 0, 0),
				VK_API_VERSION_1_4
		};

		// 填充调试报告回调创建信息结构体
		VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfoExt = {
				VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
				nullptr,
				VK_DEBUG_REPORT_WARNING_BIT_EXT
						| VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
						| VK_DEBUG_REPORT_ERROR_BIT_EXT,
				VkDebugReportCallback
		};
		// 3. create instance
		VkInstanceCreateInfo instanceInfo = {
			VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			bShouldValidate ? &debugReportCallbackInfoExt : nullptr,
			0,
			&applicationInfo,
			enableLayerCount,
			enableLayerCount > 0 ? enableLayerNames : nullptr,
			enableExtensionCount,
			enableExtensionCount > 0 ? enableExtensions : nullptr
		};
		CALL_VK(vkCreateInstance(&instanceInfo, nullptr, &mInstance));
		LOG_T("{0} : instance : {1}", __FUNCTION__, (void*)mInstance);
	}



	// 创建表面
	void AdVKGraphicContext::CreateSurface(AdWindow* window) {
		// 检查窗口是否存在
		if (!window) {
			LOG_E("window is not exists.");
			return;
		}
		// 将窗口动态转换为GLFW窗口类型
		auto* glfWwindow = dynamic_cast<AdGlfwWindow*>(window);
		if (!glfWwindow) {
			// FIXME: 处理非GLFW窗口的情况
			LOG_E("this window is not a glfw window.");
			return;
		}
		// 获取GLFW窗口的实现指针
		GLFWwindow* implWindowPointer = static_cast<GLFWwindow*>(glfWwindow->GetGLFWWindow());
		// 创建Vulkan表面
		CALL_VK(glfwCreateWindowSurface(mInstance, implWindowPointer, nullptr, &mSurface));
		// 日志输出表面信息
		LOG_T("{0} : surface : {1}", __FUNCTION__, (void*)mSurface);
	}

	// 选择物理设备
	void AdVKGraphicContext::SelectPhyDevice() {
		// 获取物理设备数量
		uint32_t phyDeviceCount;
		CALL_VK(vkEnumeratePhysicalDevices(mInstance, &phyDeviceCount, nullptr));

		// 获取所有物理设备
		std::vector<VkPhysicalDevice> phyDevices(phyDeviceCount);
		CALL_VK(vkEnumeratePhysicalDevices(mInstance, &phyDeviceCount, phyDevices.data()));

		// 初始化最大分数和对应的物理设备索引
		uint32_t maxScore = 0;
		int32_t maxScorePhyDeviceIndex = -1;

		// 日志输出物理设备信息
		LOG_D("-----------------------------");
		LOG_D("Physical devices: ");

		for (int i = 0; i < phyDeviceCount; i++) {
			// 获取当前物理设备
			VkPhysicalDevice device = phyDevices[i];

			// 获取并打印物理设备信息
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(device, &props);
			PrintPhyDeviceInfo(props);

			// 计算物理设备分数
			uint32_t score = GetPhyDeviceScore(props);

			// 获取表面格式
			uint32_t formatCount;
			CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, nullptr));
			std::vector<VkSurfaceFormatKHR> formats(formatCount);
			CALL_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, formats.data()));

			// 如果支持目标格式，则加分
			for (int j = 0; j < formatCount; j++) {
				if (formats[j].format == VK_FORMAT_B8G8R8A8_UNORM &&
					formats[j].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
					score += 10;
					break;
				}
			}

			// 获取队列族信息
			uint32_t queueFamilyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilys(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilys.data());

			// 局部变量：保存当前设备的图形和显示队列族信息
			QueueFamilyInfo graphicQueue = { -1, 0 };
			QueueFamilyInfo presentQueue = { -1, 0 };

			// 遍历所有队列族，查找图形和显示队列
			for (int j = 0; j < queueFamilyCount; j++) {
				if (queueFamilys[j].queueCount == 0) {
					continue;
				}

				// 查找图形队列族
				if (queueFamilys[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					graphicQueue.queueFamilyIndex = j;
					graphicQueue.queueCount = queueFamilys[j].queueCount;
				}

				// 查找支持显示的队列族
				VkBool32 bSupportSurface = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, j, mSurface, &bSupportSurface);
				if (bSupportSurface) {
					presentQueue.queueFamilyIndex = j;
					presentQueue.queueCount = queueFamilys[j].queueCount;
				}

				// 如果已经找到图形和显示队列族，提前退出循环（优化）
				if (graphicQueue.queueFamilyIndex >= 0 && presentQueue.queueFamilyIndex >= 0) {
					break;
				}
			}

			// 日志输出分数和队列族信息
			LOG_D("score    --->    : {0}", score);
			LOG_D("queue family     : {0}", queueFamilyCount);

			// 如果当前设备同时支持图形和显示队列族，并且分数更高，则记录
			if (graphicQueue.queueFamilyIndex >= 0 && presentQueue.queueFamilyIndex >= 0) {
				if (score > maxScore) {
					maxScore = score;
					maxScorePhyDeviceIndex = i;

					// 只有在设备满足条件时才更新类成员变量
					mGraphicQueueFamily = graphicQueue;
					mPresentQueueFamily = presentQueue;
					mPhyDevice = device;
					vkGetPhysicalDeviceMemoryProperties(device, &mPhyDeviceMemProperties);
				}
			}
		}

		// 如果没有找到合适的设备，默认使用第一个设备
		if (maxScorePhyDeviceIndex < 0) {
			LOG_W("Maybe can not find a suitable physical device, will use index 0.");
			maxScorePhyDeviceIndex = 0;

			//  即使默认设备也要重新获取队列族信息
			VkPhysicalDevice device = phyDevices[0];
			uint32_t queueFamilyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilys(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilys.data());

			QueueFamilyInfo graphicQueue = { -1, 0 };
			QueueFamilyInfo presentQueue = { -1, 0 };

			for (int j = 0; j < queueFamilyCount; j++) {
				if (queueFamilys[j].queueCount == 0) {
					continue;
				}

				if (queueFamilys[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					graphicQueue.queueFamilyIndex = j;
					graphicQueue.queueCount = queueFamilys[j].queueCount;
				}

				VkBool32 bSupportSurface = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, j, mSurface, &bSupportSurface);
				if (bSupportSurface) {
					presentQueue.queueFamilyIndex = j;
					presentQueue.queueCount = queueFamilys[j].queueCount;
				}

				if (graphicQueue.queueFamilyIndex >= 0 && presentQueue.queueFamilyIndex >= 0) {
					break;
				}
			}

			// 即使是默认设备，也必须赋值队列族信息
			mGraphicQueueFamily = graphicQueue;
			mPresentQueueFamily = presentQueue;
			mPhyDevice = device;
			vkGetPhysicalDeviceMemoryProperties(device, &mPhyDeviceMemProperties);
		}

		// 日志输出最终选择的物理设备信息
		LOG_T("{0} : physical device: {1}, score: {2}, graphic queue: {3} : {4}, present queue: {5} : {6}",
			__FUNCTION__, maxScorePhyDeviceIndex, maxScore,
			mGraphicQueueFamily.queueFamilyIndex, mGraphicQueueFamily.queueCount,
			mPresentQueueFamily.queueFamilyIndex, mPresentQueueFamily.queueCount);
	}

	// 打印物理设备信息
	void AdVKGraphicContext::PrintPhyDeviceInfo(VkPhysicalDeviceProperties& props) {
		// 根据设备类型获取设备名称
		const char* deviceType = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? "integrated gpu" :
			props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "discrete gpu" :
			props.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU ? "virtual gpu" :
			props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU ? "cpu" : "others";

		// 解析驱动版本号
		uint32_t driverVersionMajor = VK_VERSION_MAJOR(props.driverVersion);
		uint32_t driverVersionMinor = VK_VERSION_MINOR(props.driverVersion);
		uint32_t driverVersionPatch = VK_VERSION_PATCH(props.driverVersion);

		// 解析API版本号
		uint32_t apiVersionMajor = VK_VERSION_MAJOR(props.apiVersion);
		uint32_t apiVersionMinor = VK_VERSION_MINOR(props.apiVersion);
		uint32_t apiVersionPatch = VK_VERSION_PATCH(props.apiVersion);

		// 日志输出物理设备详细信息
		LOG_D("-----------------------------");
		LOG_D("deviceName       : {0}", props.deviceName);
		LOG_D("deviceType       : {0}", deviceType);
		LOG_D("vendorID         : {0}", props.vendorID);
		LOG_D("deviceID         : {0}", props.deviceID);
		LOG_D("driverVersion    : {0}.{1}.{2}", driverVersionMajor, driverVersionMinor, driverVersionPatch);
		LOG_D("apiVersion       : {0}.{1}.{2}", apiVersionMajor, apiVersionMinor, apiVersionPatch);
	}

	// 获取物理设备分数
	uint32_t AdVKGraphicContext::GetPhyDeviceScore(VkPhysicalDeviceProperties& props) {
		// 根据设备类型计算分数
		VkPhysicalDeviceType deviceType = props.deviceType;
		uint32_t score = 0;
		switch (deviceType) {
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			score += 40;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			score += 30;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			score += 20;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			score += 10;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:
			break;
		}
		return score;
	}
}