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
#include"AdGeometryUtil.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>


int main() {
	ade::Adlog::Init();
	LOG_T("{0},{1},{2}", __FUNCTION__, 1, 0.14f, true);

	std::unique_ptr<ade::AdWindow> window = ade::AdWindow::Create(800, 600, "sandbox");
	std::unique_ptr<ade::AdGraphicContext> graphicContext = ade::AdGraphicContext::Create(window.get());
	auto vkgraaphicContext = dynamic_cast<ade::AdVKGraphicContext*>(graphicContext.get());
	auto vkdevice = std::make_unique<ade::AdVKDevice>(vkgraaphicContext, 2, 2);
	auto vkswapchain = std::make_unique<ade::AdVKSwapchain>(vkgraaphicContext,vkdevice.get());
	vkswapchain->ReCreate();

	//RenderPass

	
	VkFormat depthFormat = vkdevice->GetSettings().depthFormat;

	std::vector<ade::Attachment> attachments = {
	    {
		.format = vkswapchain->GetSurfaceInfo().surfaceFormat.format,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
	    },
	    {
		.format = vkdevice->GetSettings().depthFormat,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
	    }
	};
	std::vector<ade::RenderSubPass> subpasses = {
	    {
		.colorAttachments = { 0 },
		.depthStencilAttachments = { 1 },
		.sampleCount = VK_SAMPLE_COUNT_1_BIT
	    }
	};
	// Ӧ��ʹ���Զ��帽������ͨ��������Ⱦͨ��
	auto vkRenderPass = std::make_unique<ade::AdVKRenderPass>(
		vkdevice.get(),
		attachments,  // ����������ĸ���
		subpasses     // �������������ͨ��
	);
	// ��ȡ������ͼ��
	std::vector<VkImage> swapchainImages = vkswapchain->GetImages();
	uint32_t swapchainImageCount = swapchainImages.size();
	//����images
	VkExtent3D extent = {
		    vkswapchain->GetWidth(),
		    vkswapchain->GetHeight(),
		    1
	};
	// ����֡���� (Framebuffers)
	
	std::vector<std::shared_ptr<ade::AdVKFrameBuffer>> framebuffers;
	auto depthimage = std::make_shared<ade::AdVKImage>(vkdevice.get(), extent, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	
        for (uint32_t i = 0; i < swapchainImageCount; i++)
	{ 
		std::vector<std::shared_ptr<ade::AdVKImage>> images = {
			std::make_shared<ade::AdVKImage>(vkdevice.get(),swapchainImages[i], extent, vkdevice->GetSettings().surfaceFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,VK_SAMPLE_COUNT_4_BIT),
			depthimage
			
		};
		framebuffers.push_back(std::make_shared<ade::AdVKFrameBuffer>(vkdevice.get(),vkRenderPass.get(), images, extent.width, extent.height));
		
	}
	

	
	
	struct AdVertex {
		glm::vec3 pos;
		glm::vec2 tex;
		glm::vec3 nor;
	};
	struct PushConstants {
		glm::mat4 matrix{1.f};
		uint32_t colorType = 0;
	};
	PushConstants pc;

	ade::ShaderLayout shaderlayout = {
		.pushConstants = {
			{
				VK_SHADER_STAGE_VERTEX_BIT,
				0,
				sizeof(PushConstants),
			}
		}
	};
	// �������߲��� (PipelineLayout)
	std::shared_ptr<ade::AdVKPipelineLayout> pipelineLayout =
		std::make_shared<ade::AdVKPipelineLayout>(
			vkdevice.get(),
			AD_RES_SHADER_DIR "01_hello_buffer.vert",
			AD_RES_SHADER_DIR "01_hello_buffer.frag",
			shaderlayout
		);

	// ����ͼ�ι��� (Pipeline)
	std::shared_ptr<ade::AdVKPipeline> pipeline =
		std::make_shared<ade::AdVKPipeline>(
			vkdevice.get(),
			vkRenderPass.get(),
			pipelineLayout.get()
		);
	/*uint32_t             binding;
	uint32_t             stride;
	VkVertexInputRate    inputRate;*/
	std::vector<VkVertexInputBindingDescription> vertexBindings = {
		{
			0,
			sizeof(AdVertex),
			VK_VERTEX_INPUT_RATE_VERTEX
		}
	};
	/*uint32_t    location;
	uint32_t    binding;
	VkFormat    format;
	uint32_t    offset;*/
	std::vector<VkVertexInputAttributeDescription> vertexAttrs = {
		{
			0,
			0,
                        VK_FORMAT_R32G32B32_SFLOAT,
                        offsetof(AdVertex, pos)
		},
		{
			1,
			0,
			VK_FORMAT_R32G32_SFLOAT,
			offsetof(AdVertex, tex)
		},
		{
			2,
			0,
			VK_FORMAT_R32G32B32_SFLOAT,
			offsetof(AdVertex, nor)
		},
	};
	// ���ù���״̬
	pipeline->SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		->EnableDepthTest()
		->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
	pipeline->SetVertexInputState(vertexBindings, vertexAttrs);
	pipeline->Create();

	// ��������غ��������
	std::shared_ptr<ade::AdVKCommandPool> cmdPool =
		std::make_shared<ade::AdVKCommandPool>(
			vkdevice.get(),
			vkgraaphicContext->GetGraphicQueueFamilyInfo().queueFamilyIndex
		);

	std::vector<VkCommandBuffer> cmdBuffers =
		cmdPool->AllocateCommandBuffer(static_cast<uint32_t>(swapchainImages.size()));

	std::vector<ade::AdVertex> vertices;
	std::vector<uint32_t> indices;
	ade::AdGeometryUtil::CreateCube(0.4f, -0.4f, 0.4f, -0.4f, 0.4f, -0.4f, vertices, indices);


	std::shared_ptr	<ade::AdVKBuffer> indexBuffer = std::make_shared<ade::AdVKBuffer>(
		vkdevice.get(),
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		indices.size() * sizeof(uint32_t),
		const_cast<void*>(static_cast<const void*>(indices.data())),
		false
	);


	std::unique_ptr<ade::AdVKBuffer> vertexVKBuffer = std::make_unique<ade::AdVKBuffer>(
		vkdevice.get(),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		vertices.size() * sizeof(ade::AdVertex),
		const_cast<void*>(static_cast<const void*>(vertices.data())),
		false
	);
	ade::AdVKQueue* graphicQueue = vkdevice->GetFirstGraphicQueue();
	const std::vector<VkClearValue> clearValues = {
		{
			.color = {
				.float32 = { 0.0f, 0.0f, 0.0f, 1.0f }
			}
		},
		{
			.depthStencil = {
				.depth = 1.0f,
				.stencil = 0
			}
		}
	};


	// ����ͬ�������ź�����Χ����
	const uint32_t syncObjectCount = swapchainImages.size();
	struct syncObjects {
		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;
		VkFence inFlightFence;
	};
	std::vector<syncObjects> sync(syncObjectCount);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // ��ʼΪ�Ѵ���״̬

	for (auto& syncsub : sync) {
		CALL_VK(vkCreateSemaphore(vkdevice->GetHandle(), &semaphoreInfo, nullptr, &syncsub.imageAvailableSemaphore));
		CALL_VK(vkCreateSemaphore(vkdevice->GetHandle(), &semaphoreInfo, nullptr, &syncsub.renderFinishedSemaphore));
		CALL_VK(vkCreateFence(vkdevice->GetHandle(), &fenceInfo, nullptr, &syncsub.inFlightFence));
	
	}
	
	// �������� - �����ε��������� (x, y, z, r, g, b)
	/*struct Vertex {
		float pos[3];
		float color[3];
	};*/
	
	
	// �������㻺����
	VkBuffer vertexBuffer = vertexVKBuffer->GetHandle();
	VkDeviceMemory vertexBufferMemory = vertexVKBuffer->GetMemory();
	
	uint32_t currentFrame = 0;
	// ����ѭ���ⶨ��һ���Ƕȱ���
	float baseAngleY = glm::radians(0.0f); // ��Y��25��
	float baseAngleX = glm::radians(25.0f); // ��X��30��
	float angle = 0.0f;

	// ��ʼ����
	pc.matrix = glm::rotate(glm::mat4(1.0f), baseAngleY, glm::vec3(0, 1, 0));
	pc.matrix = glm::rotate(pc.matrix, baseAngleX, glm::vec3(1, 0, 0));
	// ����Ⱦѭ���������־���
	LOG_T("Swapchain size: {0}x{1}", vkswapchain->GetWidth(), vkswapchain->GetHeight());
	

	while (!window->ShouldClose()) {
		window->PollEvents();

		// ÿ֡�����Ƕ�
		angle += 0.001f; // ������ת�ٶ�

		// �ȳ�ʼ��ת���ٶ�̬��ת
		glm::mat4 m = glm::rotate(glm::mat4(1.0f), baseAngleY, glm::vec3(0, 1, 0));
		m = glm::rotate(m, baseAngleX, glm::vec3(1, 0, 0));
		m = glm::rotate(m, angle, glm::vec3(0, 1, 0)); // ��̬��Y����ת

		pc.matrix = m;

		// 1. �ȴ���һ֡���
		vkWaitForFences(vkdevice->GetHandle(), 1, &sync[currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(vkdevice->GetHandle(), 1, &sync[currentFrame].inFlightFence);
		


		// 2. ��ȡ��ǰ������ͼ��
		uint32_t imageIndex;

		VkResult result = vkswapchain->AcquireImage(imageIndex, sync[currentFrame].imageAvailableSemaphore, VK_NULL_HANDLE);
		

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
			vkResetFences(vkdevice->GetHandle(), 1, &sync[currentFrame].inFlightFence);
			LOG_T("framebuffers recreated");
			continue; // ������һ֡����ʼ��һ֡
		}
		

		// 3. ¼���������
		VkCommandBuffer cmdbuffer1 = cmdBuffers[imageIndex];
		ade::AdVKFrameBuffer* framebuffer1 = framebuffers[imageIndex].get();


		// ���ò���ʼ¼��
		vkResetCommandBuffer(cmdbuffer1, 0);

		ade::AdVKCommandPool::BeginCommandBuffer(cmdbuffer1);
		

		// 4. ��ʼ��Ⱦͨ��
		vkRenderPass->Begin(cmdbuffer1, framebuffer1, clearValues);
	



		// 5. ��ͼ�ι���
		pipeline->Bind(cmdbuffer1);
	



		// 6. �󶨶��㻺����
		
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmdbuffer1, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(cmdbuffer1, indexBuffer->GetHandle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdPushConstants(cmdbuffer1,pipelineLayout->GetHandle(),VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(pc),&pc);
		



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
		
		
		// ��ӡǰ��������
		/*LOG_T("Vertex 0: Pos=({0},{1},{2}), Normal=({3},{4},{5})",
			vertices[0].pos.x, vertices[0].pos.y, vertices[0].pos.z,
			vertices[0].nor.x, vertices[0].nor.y, vertices[0].nor.z);*/

		// 8. ����
		/*vkCmdDraw(cmdbuffer1, static_cast<uint32_t>(vertices.size()), 1, 0, 0);*/
		vkCmdDrawIndexed(cmdbuffer1, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		


		// 9. ������Ⱦͨ��
		vkRenderPass->End(cmdbuffer1);
		


		// 10. ��������¼��
                ade::AdVKCommandPool::EndCommandBuffer(cmdbuffer1);

		// �ȴ�ͼ�����
		std::vector<VkSemaphore> waitSemaphores = { sync[currentFrame].imageAvailableSemaphore };

		// ��Ⱦ��ɺ󴥷��ź�
		std::vector<VkSemaphore> signalSemaphores = { sync[currentFrame].renderFinishedSemaphore };
		


		// 11. �ύ���ͼ�ζ���
		graphicQueue->Submit({cmdbuffer1}, waitSemaphores, signalSemaphores, sync[currentFrame].inFlightFence);
		


		// 12. ����ͼ��
		vkswapchain->Present(imageIndex, signalSemaphores);

		

		window->SwapBuffer();

		currentFrame = (currentFrame + 1) % syncObjectCount;
		
	}
	LOG_T("Loop exited normally");
	for (auto syncsub : sync) {
		vkDestroySemaphore(vkdevice->GetHandle(), syncsub.imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(vkdevice->GetHandle(), syncsub.renderFinishedSemaphore, nullptr);
		vkDestroyFence(vkdevice->GetHandle(), syncsub.inFlightFence, nullptr);
	}
	
	

	return EXIT_SUCCESS;
	
}