#include "Render/AdRenderContext.h"

namespace ade {
	/**
 * @brief AdRenderContext���캯��
 * @param window ���ڶ���ָ�룬���ڴ���ͼ��������
 *
 * �ù��캯����ʼ����Ⱦ�����ģ���������ͼ�������ġ��豸����ͽ�����
 */
	AdRenderContext::AdRenderContext(AdWindow* window) {
		// ����ͼ��������
		mGraphicContext = ade::AdGraphicContext::Create(window);
		// ��ȡVulkanͼ��������
		auto vkContext = dynamic_cast<ade::AdVKGraphicContext*>(mGraphicContext.get());
		// ����Vulkan�豸����
		mDevice = std::make_shared<ade::AdVKDevice>(vkContext, 1, 1);
		// ����Vulkan������
		mSwapchain = std::make_shared<ade::AdVKSwapchain>(vkContext, mDevice.get());
	}

	AdRenderContext::~AdRenderContext() {

	}
}