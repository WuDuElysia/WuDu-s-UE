#include "Render/AdRenderContext.h"

namespace WuDu {
	/**
 * @brief AdRenderContext构造函数
 * @param window 窗口指针，用于创建图形上下文
 *
 * 该构造函数初始化渲染上下文模块，包括图形上下文、设备设置和交换链
 */
	AdRenderContext::AdRenderContext(AdWindow* window) {
		// 创建图形上下文
		mGraphicContext = WuDu::AdGraphicContext::Create(window);
		// 获取Vulkan图形上下文
		auto vkContext = dynamic_cast<WuDu::AdVKGraphicContext*>(mGraphicContext.get());
		// 创建Vulkan设备设置
		mDevice = std::make_shared<WuDu::AdVKDevice>(vkContext, 1, 1);
		// 创建Vulkan交换链
		mSwapchain = std::make_shared<WuDu::AdVKSwapchain>(vkContext, mDevice.get());
	}

	AdRenderContext::~AdRenderContext() {

	}
}