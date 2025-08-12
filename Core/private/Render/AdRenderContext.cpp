#include "Render/AdRenderContext.h"

namespace ade {
	/**
 * @brief AdRenderContext构造函数
 * @param window 窗口对象指针，用于创建图形上下文
 *
 * 该构造函数初始化渲染上下文，包括创建图形上下文、设备对象和交换链
 */
	AdRenderContext::AdRenderContext(AdWindow* window) {
		// 创建图形上下文
		mGraphicContext = ade::AdGraphicContext::Create(window);
		// 获取Vulkan图形上下文
		auto vkContext = dynamic_cast<ade::AdVKGraphicContext*>(mGraphicContext.get());
		// 创建Vulkan设备对象
		mDevice = std::make_shared<ade::AdVKDevice>(vkContext, 1, 1);
		// 创建Vulkan交换链
		mSwapchain = std::make_shared<ade::AdVKSwapchain>(vkContext, mDevice.get());
	}

	AdRenderContext::~AdRenderContext() {

	}
}