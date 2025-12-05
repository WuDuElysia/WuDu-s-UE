#include "Graphic/AdVKPipeline.h"
#include "AdFileUtil.h"
#include "Graphic/AdVKDevice.h"
#include "Graphic/AdVKRenderPass.h"

namespace WuDu {
	/**
	* @brief 构造函数，创建Vulkan管线布局对象
	*
	* 该构造函数负责编译着色器模块并创建Vulkan管线布局。它会加载顶点着色器和片段着色器，
	* 然后根据提供的着色器布局信息创建管线布局。
	*
	* @param device 指向Vulkan设备对象的指针，用于创建Vulkan资源
	* @param vertexShaderFile 顶点着色器文件路径（不包含.spv扩展名）
	* @param fragShaderFile 片段着色器文件路径（不包含.spv扩展名）
	* @param shaderLayout 着色器布局信息，包含描述符集布局和推送常量范围
	*/
	AdVKPipelineLayout::AdVKPipelineLayout(AdVKDevice* device, const std::string& vertexShaderFile, const std::string& fragShaderFile, const ShaderLayout& shaderLayout) : mDevice(device) {
		// 编译着色器模块
		CALL_VK(CreateShaderModule(vertexShaderFile + ".spv", &mVertexShaderModule));
		CALL_VK(CreateShaderModule(fragShaderFile + ".spv", &mFragShaderModule));

		// 创建管线布局
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = static_cast<uint32_t>(shaderLayout.descriptorSetLayouts.size()),
			.pSetLayouts = shaderLayout.descriptorSetLayouts.data(),
			.pushConstantRangeCount = static_cast<uint32_t>(shaderLayout.pushConstants.size()),
			.pPushConstantRanges = shaderLayout.pushConstants.data()
		};
		CALL_VK(vkCreatePipelineLayout(mDevice->GetHandle(), &pipelineLayoutInfo, nullptr, &mHandle));
	}

	/**
	* @brief 析构函数，用于清理管线布局相关的 Vulkan 资源
	*
	* 该函数会销毁顶点着色器模块、片段着色器模块以及管线布局对象。
	* 使用宏 VK_D 来调用对应的 Vulkan 销毁函数。
	*/
	AdVKPipelineLayout::~AdVKPipelineLayout() {
		VK_D(ShaderModule, mDevice->GetHandle(), mVertexShaderModule);
		VK_D(ShaderModule, mDevice->GetHandle(), mFragShaderModule);
		VK_D(PipelineLayout, mDevice->GetHandle(), mHandle);
	}

	/**
	 * @brief 创建 Vulkan 着色器模块
	 *
	 * 从指定文件路径读取着色器代码，并创建对应的 Vulkan 着色器模块对象。
	 *
	 * @param filePath 着色器代码文件的路径
	 * @param outShaderModule 指向用于存储创建的着色器模块句柄的指针
	 * @return VkResult 返回 Vulkan API 的结果代码，VK_SUCCESS 表示成功
	 */
	VkResult AdVKPipelineLayout::CreateShaderModule(const std::string& filePath, VkShaderModule* outShaderModule) {
		// 从文件中读取着色器代码内容
		std::vector<char> content = ReadCharArrayFromFile(filePath);

		// 配置着色器模块创建信息结构体
		VkShaderModuleCreateInfo shaderModuleInfo = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = static_cast<uint32_t>(content.size()),
			.pCode = reinterpret_cast<const uint32_t*>(content.data())
		};

		// 调用 Vulkan API 创建着色器模块
		return vkCreateShaderModule(mDevice->GetHandle(), &shaderModuleInfo, nullptr, outShaderModule);
	}

	////// Pipeline

	AdVKPipeline::AdVKPipeline(AdVKDevice* device, AdVKRenderPass* renderPass, AdVKPipelineLayout* pipelineLayout) : mDevice(device), mRenderPass(renderPass), mPipelineLayout(pipelineLayout) {

	}

	AdVKPipeline::~AdVKPipeline() {
		VK_D(Pipeline, mDevice->GetHandle(), mHandle);
	}

	/**
	* @brief 创建 Vulkan 图形管线
	*
	* 该函数负责创建 Vulkan 图形渲染管线，包括配置顶点着色器和片段着色器阶段。
	* 管线创建需要预先设置好的着色器模块和管线布局。
	*
	* @note 该函数不返回任何值，但会更新对象内部的管线状态
	*/
	void AdVKPipeline::Create() {
		// 配置图形管线的着色器阶段信息
		// 包括顶点着色器阶段和片段着色器阶段的创建信息
		VkPipelineShaderStageCreateInfo shaderStageInfo[] = {
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = mPipelineLayout->GetVertexShaderModule(),
				.pName = "main",
				.pSpecializationInfo = nullptr
			},
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = mPipelineLayout->GetFragShaderModule(),
				.pName = "main",
				.pSpecializationInfo = nullptr
			}
		};

		VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.vertexBindingDescriptionCount = static_cast<uint32_t>(mPipelineConfig.vertexInputState.vertexBindings.size()),
			.pVertexBindingDescriptions = mPipelineConfig.vertexInputState.vertexBindings.data(),
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(mPipelineConfig.vertexInputState.vertexAttributes.size()),
			.pVertexAttributeDescriptions = mPipelineConfig.vertexInputState.vertexAttributes.data()
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.topology = mPipelineConfig.inputAssemblyState.topology,
			.primitiveRestartEnable = mPipelineConfig.inputAssemblyState.primitiveRestartEnable
		};

		VkViewport defaultViewport = {
			.x = 0,
			.y = 0,
			.width = 100,
			.height = 100,
			.minDepth = 0,
			.maxDepth = 1
		};
		VkRect2D defaultScissor = {
			.offset = { 0, 0 },
			.extent = { 100, 100 }
		};
		VkPipelineViewportStateCreateInfo viewportStateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.viewportCount = 1,
			.pViewports = &defaultViewport,
			.scissorCount = 1,
			.pScissors = &defaultScissor
		};

		VkPipelineRasterizationStateCreateInfo rasterizationStateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthClampEnable = mPipelineConfig.rasterizationState.depthClampEnable,
			.rasterizerDiscardEnable = mPipelineConfig.rasterizationState.rasterizerDiscardEnable,
			.polygonMode = mPipelineConfig.rasterizationState.polygonMode,
			.cullMode = mPipelineConfig.rasterizationState.cullMode,
			.frontFace = mPipelineConfig.rasterizationState.frontFace,
			.depthBiasEnable = mPipelineConfig.rasterizationState.depthBiasEnable,
			.depthBiasConstantFactor = mPipelineConfig.rasterizationState.depthBiasConstantFactor,
			.depthBiasClamp = mPipelineConfig.rasterizationState.depthBiasClamp,
			.depthBiasSlopeFactor = mPipelineConfig.rasterizationState.depthBiasSlopeFactor,
			.lineWidth = mPipelineConfig.rasterizationState.lineWidth
		};

		VkPipelineMultisampleStateCreateInfo multisampleStateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.rasterizationSamples = mPipelineConfig.multisampleState.rasterizationSamples,
			.sampleShadingEnable = mPipelineConfig.multisampleState.sampleShadingEnable,
			.minSampleShading = mPipelineConfig.multisampleState.minSampleShading,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE
		};

		VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthTestEnable = mPipelineConfig.depthStencilState.depthTestEnable,
			.depthWriteEnable = mPipelineConfig.depthStencilState.depthWriteEnable,
			.depthCompareOp = mPipelineConfig.depthStencilState.depthCompareOp,
			.depthBoundsTestEnable = mPipelineConfig.depthStencilState.depthBoundsTestEnable,
			.stencilTestEnable = mPipelineConfig.depthStencilState.stencilTestEnable,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 0.0f
		};

		VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_CLEAR,
			.attachmentCount = 1,
			.pAttachments = &mPipelineConfig.colorBlendAttachmentState,
		};
		colorBlendStateInfo.blendConstants[0] = colorBlendStateInfo.blendConstants[1] = colorBlendStateInfo.blendConstants[2] = colorBlendStateInfo.blendConstants[3] = 0;

		VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.dynamicStateCount = static_cast<uint32_t>(mPipelineConfig.dynamicState.dynamicStates.size()),
			.pDynamicStates = mPipelineConfig.dynamicState.dynamicStates.data()
		};
		VkGraphicsPipelineCreateInfo pipelineInfo = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stageCount = ARRAY_SIZE(shaderStageInfo),
			.pStages = shaderStageInfo,
			.pVertexInputState = &vertexInputStateInfo,
			.pInputAssemblyState = &inputAssemblyStateInfo,
			.pTessellationState = nullptr,
			.pViewportState = &viewportStateInfo,
			.pRasterizationState = &rasterizationStateInfo,
			.pMultisampleState = &multisampleStateInfo,
			.pDepthStencilState = &depthStencilStateInfo,
			.pColorBlendState = &colorBlendStateInfo,
			.pDynamicState = &dynamicStateInfo,
			.layout = mPipelineLayout->GetHandle(),
			.renderPass = mRenderPass->GetHandle(),
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = 0
		};
		CALL_VK(vkCreateGraphicsPipelines(mDevice->GetHandle(), mDevice->GetPipelineCache(), 1, &pipelineInfo, nullptr, &mHandle));
		LOG_T("Create pipeline : {0}", (void*)mHandle);
	}

	AdVKPipeline* AdVKPipeline::SetVertexInputState(const std::vector<VkVertexInputBindingDescription>& vertexBindings,
		const std::vector<VkVertexInputAttributeDescription>& vertexAttrs) {
		mPipelineConfig.vertexInputState.vertexBindings = vertexBindings;
		mPipelineConfig.vertexInputState.vertexAttributes = vertexAttrs;
		return this;
	}

	AdVKPipeline* AdVKPipeline::SetInputAssemblyState(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable) {
		mPipelineConfig.inputAssemblyState.topology = topology;
		mPipelineConfig.inputAssemblyState.primitiveRestartEnable = primitiveRestartEnable;
		return this;
	}

	AdVKPipeline* AdVKPipeline::SetRasterizationState(const PipelineRasterizationState& rasterizationState) {
		mPipelineConfig.rasterizationState.depthClampEnable = rasterizationState.depthClampEnable;
		mPipelineConfig.rasterizationState.rasterizerDiscardEnable = rasterizationState.rasterizerDiscardEnable;
		mPipelineConfig.rasterizationState.polygonMode = rasterizationState.polygonMode;
		mPipelineConfig.rasterizationState.cullMode = rasterizationState.cullMode;
		mPipelineConfig.rasterizationState.frontFace = rasterizationState.frontFace;
		mPipelineConfig.rasterizationState.depthBiasEnable = rasterizationState.depthBiasEnable;
		mPipelineConfig.rasterizationState.depthBiasConstantFactor = rasterizationState.depthBiasConstantFactor;
		mPipelineConfig.rasterizationState.depthBiasClamp = rasterizationState.depthBiasClamp;
		mPipelineConfig.rasterizationState.depthBiasSlopeFactor = rasterizationState.depthBiasSlopeFactor;
		mPipelineConfig.rasterizationState.lineWidth = rasterizationState.lineWidth;
		return this;
	}

	AdVKPipeline* AdVKPipeline::SetMultisampleState(VkSampleCountFlagBits samples, VkBool32 sampleShadingEnable, float minSampleShading) {
		mPipelineConfig.multisampleState.rasterizationSamples = samples;
		mPipelineConfig.multisampleState.sampleShadingEnable = sampleShadingEnable;
		mPipelineConfig.multisampleState.minSampleShading = minSampleShading;
		return this;
	}

	AdVKPipeline* AdVKPipeline::SetDepthStencilState(const PipelineDepthStencilState& depthStencilState) {
		mPipelineConfig.depthStencilState.depthTestEnable = depthStencilState.depthTestEnable;
		mPipelineConfig.depthStencilState.depthWriteEnable = depthStencilState.depthWriteEnable;
		mPipelineConfig.depthStencilState.depthCompareOp = depthStencilState.depthCompareOp;
		mPipelineConfig.depthStencilState.depthBoundsTestEnable = depthStencilState.depthBoundsTestEnable;
		mPipelineConfig.depthStencilState.stencilTestEnable = depthStencilState.stencilTestEnable;
		return this;
	}

	AdVKPipeline* AdVKPipeline::SetColorBlendAttachmentState(VkBool32 blendEnable, VkBlendFactor srcColorBlendFactor,
		VkBlendFactor dstColorBlendFactor, VkBlendOp colorBlendOp,
		VkBlendFactor srcAlphaBlendFactor,
		VkBlendFactor dstAlphaBlendFactor,
		VkBlendOp alphaBlendOp) {
		mPipelineConfig.colorBlendAttachmentState.blendEnable = blendEnable;
		mPipelineConfig.colorBlendAttachmentState.srcColorBlendFactor = srcColorBlendFactor;
		mPipelineConfig.colorBlendAttachmentState.dstColorBlendFactor = dstColorBlendFactor;
		mPipelineConfig.colorBlendAttachmentState.srcAlphaBlendFactor = srcAlphaBlendFactor;
		mPipelineConfig.colorBlendAttachmentState.dstAlphaBlendFactor = dstAlphaBlendFactor;
		mPipelineConfig.colorBlendAttachmentState.alphaBlendOp = alphaBlendOp;
		return this;
	}

	AdVKPipeline* AdVKPipeline::SetDynamicState(const std::vector<VkDynamicState>& dynamicStates) {
		mPipelineConfig.dynamicState.dynamicStates = dynamicStates;
		return this;
	}

	AdVKPipeline* AdVKPipeline::EnableAlphaBlend() {
		mPipelineConfig.colorBlendAttachmentState = {
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD
		};
		return this;
	}

	AdVKPipeline* AdVKPipeline::EnableDepthTest() {
		mPipelineConfig.depthStencilState.depthTestEnable = VK_TRUE;
		mPipelineConfig.depthStencilState.depthWriteEnable = VK_TRUE;
		mPipelineConfig.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
		return this;
	}

	void AdVKPipeline::Bind(VkCommandBuffer cmdBuffer) {
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mHandle);
	}
}