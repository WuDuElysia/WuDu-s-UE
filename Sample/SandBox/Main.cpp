#include<iostream>
#include"Adlog.h"
#include"AdWindow.h"
#include"AdGraphicContext.h"
#include"Graphic/AdVKGraphicContext.h"
#include"Graphic/AdVKDevice.h"
#include"Graphic/AdVKSwapchain.h"
#include"Graphic/AdVKRenderPass.h"
#include"Graphic/AdVKPipeLine.h"
#include"Graphic/AdVKFrameBuffer.h"
#include"Graphic/AdVKCommandBuffer.h"
#include"Graphic/AdVKImage.h"

int main() {
	ade::Adlog::Init();
	LOG_T("{0},{1},{2}", __FUNCTION__, 1, 0.14f, true);

	std::unique_ptr<ade::AdWindow> window = ade::AdWindow::Create(800, 600, "sandbox");
	std::unique_ptr<ade::AdGraphicContext> graphicContext = ade::AdGraphicContext::Create(window.get());
	auto vkgraaphicContext = dynamic_cast<ade::AdVKGraphicContext*>(graphicContext.get());
	auto vkdevice = std::make_unique<ade::AdVKDevice>(vkgraaphicContext, 2, 2);
	auto vkswapchain = std::make_unique<ade::AdVKSwapchain>(vkgraaphicContext,vkdevice.get());
	vkswapchain->ReCreate();
	auto vkRenderPass = std::make_unique<ade::AdVKRenderPass>(vkdevice.get());
	// ��ȡ������ͼ��
	std::vector<VkImage> swapchainImages = vkswapchain->GetImages();

	// ����֡���� (Framebuffers)

	std::vector<std::shared_ptr<ade::AdVKFrameBuffer>> framebuffers;
	for (const auto& image : swapchainImages) {
		// ��ȡ������ͼ�����Ϣ
		VkExtent3D extent = {
		    vkswapchain->GetWidth(),
		    vkswapchain->GetHeight(),
		    1
		};

		// �ؼ�����ȡ��ȷ��ͼ���ʽ
		VkFormat format = vkswapchain->GetSurfaceInfo().surfaceFormat.format;  // ��Ӧ���� VK_FORMAT_B8G8R8A8_UNORM
		VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		// ʹ�����еĹ��캯������ AdVKImage ���󣬲��ṩ��ȷ�ĸ�ʽ
		auto adImage = std::make_shared<ade::AdVKImage>(
			vkdevice.get(),     // device
			image,              // existing VkImage
			extent,             // extent
			format,             // format - �ؼ�����
			usage               // usage
		);

		// �������� AdVKImage ������
		std::vector<std::shared_ptr<ade::AdVKImage>> images = { adImage };

		framebuffers.push_back(std::make_shared<ade::AdVKFrameBuffer>(
			vkdevice.get(),
			vkRenderPass.get(),
			images,
			vkswapchain->GetWidth(),
			vkswapchain->GetHeight()
		));
	}

	// �������߲��� (PipelineLayout)
	std::shared_ptr<ade::AdVKPipelineLayout> pipelineLayout =
		std::make_shared<ade::AdVKPipelineLayout>(
			vkdevice.get(),
			AD_RES_SHADER_DIR "00_hello_triangle.vert",
			AD_RES_SHADER_DIR "00_hello_triangle.frag"
		);

	// ����ͼ�ι��� (Pipeline)
	std::shared_ptr<ade::AdVKPipeline> pipeline =
		std::make_shared<ade::AdVKPipeline>(
			vkdevice.get(),
			vkRenderPass.get(),
			pipelineLayout.get()
		);

	// ���ù���״̬
	pipeline->SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
		->EnableDepthTest()
		->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
	pipeline->Create();

	// ��������غ��������
	std::shared_ptr<ade::AdVKCommandPool> cmdPool =
		std::make_shared<ade::AdVKCommandPool>(
			vkdevice.get(),
			vkgraaphicContext->GetGraphicQueueFamilyInfo().queueFamilyIndex
		);

	std::vector<VkCommandBuffer> cmdBuffers =
		cmdPool->AllocateCommandBuffer(static_cast<uint32_t>(swapchainImages.size()));
	while (!window->ShouldClose()) {
		window->PollEvents();
		window->SwapBuffer();
	}
	return EXIT_SUCCESS;
	
}