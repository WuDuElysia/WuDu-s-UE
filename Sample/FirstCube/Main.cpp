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
	// 应该使用自定义附件和子通道创建渲染通道
	auto vkRenderPass = std::make_unique<ade::AdVKRenderPass>(
		vkdevice.get(),
		attachments,  // 传入您定义的附件
		subpasses     // 传入您定义的子通道
	);
	// 获取交换链图像
	std::vector<VkImage> swapchainImages = vkswapchain->GetImages();
	uint32_t swapchainImageCount = swapchainImages.size();
	//创建images
	VkExtent3D extent = {
		    vkswapchain->GetWidth(),
		    vkswapchain->GetHeight(),
		    1
	};
	// 创建帧缓冲 (Framebuffers)
	
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
	// 创建管线布局 (PipelineLayout)
	std::shared_ptr<ade::AdVKPipelineLayout> pipelineLayout =
		std::make_shared<ade::AdVKPipelineLayout>(
			vkdevice.get(),
			AD_RES_SHADER_DIR "01_hello_buffer.vert",
			AD_RES_SHADER_DIR "01_hello_buffer.frag",
			shaderlayout
		);

	// 创建图形管线 (Pipeline)
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
	// 配置管线状态
	pipeline->SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
		->EnableDepthTest()
		->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
	pipeline->SetVertexInputState(vertexBindings, vertexAttrs);
	pipeline->Create();

	// 创建命令池和命令缓冲区
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


	// 创建同步对象（信号量和围栏）
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
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // 初始为已触发状态

	for (auto& syncsub : sync) {
		CALL_VK(vkCreateSemaphore(vkdevice->GetHandle(), &semaphoreInfo, nullptr, &syncsub.imageAvailableSemaphore));
		CALL_VK(vkCreateSemaphore(vkdevice->GetHandle(), &semaphoreInfo, nullptr, &syncsub.renderFinishedSemaphore));
		CALL_VK(vkCreateFence(vkdevice->GetHandle(), &fenceInfo, nullptr, &syncsub.inFlightFence));
	
	}
	
	// 顶点数据 - 三角形的三个顶点 (x, y, z, r, g, b)
	/*struct Vertex {
		float pos[3];
		float color[3];
	};*/
	
	
	// 创建顶点缓冲区
	VkBuffer vertexBuffer = vertexVKBuffer->GetHandle();
	VkDeviceMemory vertexBufferMemory = vertexVKBuffer->GetMemory();
	
	uint32_t currentFrame = 0;
	// 在主循环外定义一个角度变量
	float baseAngleY = glm::radians(0.0f); // 绕Y轴25度
	float baseAngleX = glm::radians(25.0f); // 绕X轴30度
	float angle = 0.0f;

	// 初始矩阵
	pc.matrix = glm::rotate(glm::mat4(1.0f), baseAngleY, glm::vec3(0, 1, 0));
	pc.matrix = glm::rotate(pc.matrix, baseAngleX, glm::vec3(1, 0, 0));
	// 在渲染循环中添加日志检查
	LOG_T("Swapchain size: {0}x{1}", vkswapchain->GetWidth(), vkswapchain->GetHeight());
	

	while (!window->ShouldClose()) {
		window->PollEvents();

		// 每帧递增角度
		angle += 0.001f; // 控制旋转速度

		// 先初始旋转，再动态旋转
		glm::mat4 m = glm::rotate(glm::mat4(1.0f), baseAngleY, glm::vec3(0, 1, 0));
		m = glm::rotate(m, baseAngleX, glm::vec3(1, 0, 0));
		m = glm::rotate(m, angle, glm::vec3(0, 1, 0)); // 动态绕Y轴旋转

		pc.matrix = m;

		// 1. 等待上一帧完成
		vkWaitForFences(vkdevice->GetHandle(), 1, &sync[currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(vkdevice->GetHandle(), 1, &sync[currentFrame].inFlightFence);
		


		// 2. 获取当前交换链图像
		uint32_t imageIndex;

		VkResult result = vkswapchain->AcquireImage(imageIndex, sync[currentFrame].imageAvailableSemaphore, VK_NULL_HANDLE);
		

		// 3. 处理 swapchain 状态变化
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			LOG_T("Swapchain needs to be recreated");

			// 关键：在重建前等待设备空闲
			vkDeviceWaitIdle(vkdevice->GetHandle());

			// 重建 swapchain
			vkswapchain->ReCreate();
			LOG_T("Swapchain recreated");
			// 重建相关的 framebuffers
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
			continue; // 跳过这一帧，开始下一帧
		}
		

		// 3. 录制命令缓冲区
		VkCommandBuffer cmdbuffer1 = cmdBuffers[imageIndex];
		ade::AdVKFrameBuffer* framebuffer1 = framebuffers[imageIndex].get();


		// 重置并开始录制
		vkResetCommandBuffer(cmdbuffer1, 0);

		ade::AdVKCommandPool::BeginCommandBuffer(cmdbuffer1);
		

		// 4. 开始渲染通道
		vkRenderPass->Begin(cmdbuffer1, framebuffer1, clearValues);
	



		// 5. 绑定图形管线
		pipeline->Bind(cmdbuffer1);
	



		// 6. 绑定顶点缓冲区
		
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmdbuffer1, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(cmdbuffer1, indexBuffer->GetHandle(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdPushConstants(cmdbuffer1,pipelineLayout->GetHandle(),VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(pc),&pc);
		



		// 7. 设置视口和裁剪（动态状态）
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
		
		
		// 打印前几个顶点
		/*LOG_T("Vertex 0: Pos=({0},{1},{2}), Normal=({3},{4},{5})",
			vertices[0].pos.x, vertices[0].pos.y, vertices[0].pos.z,
			vertices[0].nor.x, vertices[0].nor.y, vertices[0].nor.z);*/

		// 8. 绘制
		/*vkCmdDraw(cmdbuffer1, static_cast<uint32_t>(vertices.size()), 1, 0, 0);*/
		vkCmdDrawIndexed(cmdbuffer1, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		


		// 9. 结束渲染通道
		vkRenderPass->End(cmdbuffer1);
		


		// 10. 结束命令录制
                ade::AdVKCommandPool::EndCommandBuffer(cmdbuffer1);

		// 等待图像可用
		std::vector<VkSemaphore> waitSemaphores = { sync[currentFrame].imageAvailableSemaphore };

		// 渲染完成后触发信号
		std::vector<VkSemaphore> signalSemaphores = { sync[currentFrame].renderFinishedSemaphore };
		


		// 11. 提交命令到图形队列
		graphicQueue->Submit({cmdbuffer1}, waitSemaphores, signalSemaphores, sync[currentFrame].inFlightFence);
		


		// 12. 呈现图像
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