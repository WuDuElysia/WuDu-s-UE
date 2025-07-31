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
#include"Graphic/AdVKBuffer.h"
#include"Graphic/AdVKQueue.h"

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
	pipeline->SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
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

	ade::AdVKQueue* graphicQueue = vkdevice->GetFirstGraphicQueue();
	// ����ͬ�������ź�����Χ����
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence inFlightFence;

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // ��ʼΪ�Ѵ���״̬

	vkCreateSemaphore(vkdevice->GetHandle(), &semaphoreInfo, nullptr, &imageAvailableSemaphore);
	vkCreateSemaphore(vkdevice->GetHandle(), &semaphoreInfo, nullptr, &renderFinishedSemaphore);
	vkCreateFence(vkdevice->GetHandle(), &fenceInfo, nullptr, &inFlightFence);

	// �������� - �����ε��������� (x, y, z, r, g, b)
	struct Vertex {
		float pos[3];
		float color[3];
	};

	const std::vector<Vertex> vertices = {
	    {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}, // ����1 (��ɫ)
	    {{ 1.0f,  1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}, // ����2 (��ɫ)
	    {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}  // ����3 (��ɫ)
	};

	// �������㻺����
	std::unique_ptr<ade::AdVKBuffer> vertexVKBuffer = std::make_unique<ade::AdVKBuffer>(
		vkdevice.get(),                                   
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,               
		vertices.size() * sizeof(Vertex),                
		const_cast<void*>(static_cast<const void*>(vertices.data())), 
		false                                             
	);
	
	VkBuffer vertexBuffer = vertexVKBuffer->GetHandle();
	VkDeviceMemory vertexBufferMemory = vertexVKBuffer->GetMemory();
	
	while (!window->ShouldClose()) {
		window->PollEvents();
		LOG_T("wait begin");
		// 1. �ȴ���һ֡���
		vkWaitForFences(vkdevice->GetHandle(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(vkdevice->GetHandle(), 1, &inFlightFence);
		LOG_T("wait");


		// 2. ��ȡ��ǰ������ͼ��
		uint32_t imageIndex;

		VkResult result = vkswapchain->AcquireImage(imageIndex, imageAvailableSemaphore, VK_NULL_HANDLE);
		LOG_T("getimage");

		// 3. ���� swapchain ״̬�仯
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			LOG_T("Swapchain needs to be recreated");

			// �ؼ������ؽ�ǰ�ȴ��豸����
			vkDeviceWaitIdle(vkdevice->GetHandle());

			// �ؽ� swapchain
			vkswapchain->ReCreate();
			LOG_T("Swapchain recreated");
			// �ؽ���ص� framebuffers
			framebuffers.clear();
			std::vector<VkImage> swapchainImages = vkswapchain->GetImages();

			for (const auto& image : swapchainImages) {
				VkExtent3D extent = {
				    vkswapchain->GetWidth(),
				    vkswapchain->GetHeight(),
				    1
				};

				VkFormat format = vkswapchain->GetSurfaceInfo().surfaceFormat.format;
				VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

				auto adImage = std::make_shared<ade::AdVKImage>(
					vkdevice.get(), image, extent, format, usage);

				std::vector<std::shared_ptr<ade::AdVKImage>> images = { adImage };

				framebuffers.push_back(std::make_shared<ade::AdVKFrameBuffer>(
					vkdevice.get(),
					vkRenderPass.get(),
					images,
					vkswapchain->GetWidth(),
					vkswapchain->GetHeight()
				));
			}
			vkResetFences(vkdevice->GetHandle(), 1, &inFlightFence);
			LOG_T("framebuffers recreated");
			continue; // ������һ֡����ʼ��һ֡
		}
		LOG_T("getswapimage");

		// 3. ¼���������
		VkCommandBuffer cmdbuffer1 = cmdBuffers[imageIndex];
		ade::AdVKFrameBuffer* framebuffer1 = framebuffers[imageIndex].get();
		

		// ���ò���ʼ¼��
		vkResetCommandBuffer(cmdbuffer1,0);

		ade::AdVKCommandPool::BeginCommandBuffer(cmdbuffer1);
		LOG_T("begincmd");

		// 4. ��ʼ��Ⱦͨ��
		std::vector<VkClearValue> clearcolor = { {0.0f, 0.0f, 0.0f, 1.0f} };
		vkRenderPass->Begin(cmdbuffer1,framebuffer1,clearcolor);
		LOG_T("beginrenderpass");


		// 5. ��ͼ�ι���
		pipeline->Bind(cmdbuffer1);
		LOG_T("binpipe");



		// 6. �󶨶��㻺����
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmdbuffer1, 0, 1, vertexBuffers, offsets);
		LOG_T("bindvertex");



		// 7. �����ӿںͲü�����̬״̬��
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(vkswapchain->GetWidth());
		viewport.height = static_cast<float>(vkswapchain->GetHeight());
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmdbuffer1, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = { vkswapchain->GetWidth(), vkswapchain->GetHeight() };
		vkCmdSetScissor(cmdbuffer1, 0, 1, &scissor);
		LOG_T("setview");



		// 8. ���������Σ�
		vkCmdDraw(cmdbuffer1, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
		LOG_T("drow");


		// 9. ������Ⱦͨ��
		vkRenderPass->End(cmdbuffer1);
		LOG_T("endrenderpass");


		// 10. ��������¼��
                ade::AdVKCommandPool::EndCommandBuffer(cmdbuffer1);

		// �ȴ�ͼ�����
		std::vector<VkSemaphore> waitSemaphores = { imageAvailableSemaphore };

		// ��Ⱦ��ɺ󴥷��ź�
		std::vector<VkSemaphore> signalSemaphores = { renderFinishedSemaphore };
		LOG_T("endcmd");


		// 11. �ύ���ͼ�ζ���
		graphicQueue->Submit({cmdbuffer1}, waitSemaphores, signalSemaphores,inFlightFence);
		LOG_T("submit");


		// 12. ����ͼ��
		vkswapchain->Present(imageIndex, signalSemaphores);

		LOG_T("prsent");

		window->SwapBuffer();
		LOG_T("Frame completed");
	}
	LOG_T("Loop exited normally");
	vkDestroySemaphore(vkdevice->GetHandle(), imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(vkdevice->GetHandle(), renderFinishedSemaphore, nullptr);
	vkDestroyFence(vkdevice->GetHandle(), inFlightFence, nullptr);
	vkDestroyBuffer(vkdevice->GetHandle(), vertexBuffer, nullptr);
	vkFreeMemory(vkdevice->GetHandle(), vertexBufferMemory, nullptr);

	return EXIT_SUCCESS;
	
}