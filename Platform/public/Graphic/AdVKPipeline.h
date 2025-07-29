#ifndef AD_VKPIPELINE_H
#define AD_VKPIPELINE_H

#include "AdVKCommon.h"

namespace ade {
	class AdVKDevice;
	class AdVKRenderPass;

	/**
	 * @brief 描述着色器的布局信息，包括描述符集合布局和推送常量范围
	 */
	struct ShaderLayout {
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts; ///< 描述符集布局数组
		std::vector<VkPushConstantRange> pushConstants;          ///< 推送常量范围数组
	};

	/**
	 * @brief 管线顶点输入状态配置结构体
	 */
	struct PipelineVertexInputState {
		std::vector<VkVertexInputBindingDescription> vertexBindings;     ///< 顶点绑定描述数组
		std::vector<VkVertexInputAttributeDescription> vertexAttributes; ///< 顶点属性描述数组
	};

	/**
	 * @brief 管线输入装配状态配置结构体
	 */
	struct PipelineInputAssemblyState {
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; ///< 图元拓扑类型，默认为三角形列表
		VkBool32 primitiveRestartEnable = VK_FALSE;                         ///< 是否启用图元重启功能，默认关闭
	};

	/**
	 * @brief 管线光栅化状态配置结构体
	 */
	struct PipelineRasterizationState {
		VkBool32 depthClampEnable = VK_FALSE;                ///< 是否启用深度截取，默认关闭
		VkBool32 rasterizerDiscardEnable = VK_FALSE;         ///< 是否丢弃光栅化结果，默认不丢弃
		VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;    ///< 多边形填充模式，默认填充
		VkCullModeFlags cullMode = VK_CULL_MODE_NONE;        ///< 面剔除模式，默认不剔除
		VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; ///< 前面绕向，默认逆时针
		VkBool32 depthBiasEnable = VK_FALSE;                 ///< 是否启用深度偏移，默认关闭
		float depthBiasConstantFactor = 0;                   ///< 深度偏移常量因子
		float depthBiasClamp = 0;                            ///< 深度偏移钳位值
		float depthBiasSlopeFactor = 0;                      ///< 深度偏移斜率因子
		float lineWidth = 1.f;                               ///< 线宽，默认为1.0
	};

	/**
	 * @brief 管线多重采样状态配置结构体
	 */
	struct PipelineMultisampleState {
		VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; ///< 光栅化采样数，默认为1个样本
		VkBool32 sampleShadingEnable = VK_FALSE;                            ///< 是否启用样本着色，默认关闭
		float minSampleShading = 0.2f;                                      ///< 最小样本着色比例
	};

	/**
	 * @brief 管线深度和模板测试状态配置结构体
	 */
	struct PipelineDepthStencilState {
		VkBool32 depthTestEnable = VK_FALSE;       ///< 是否启用深度测试，默认关闭
		VkBool32 depthWriteEnable = VK_FALSE;      ///< 是否启用深度写入，默认关闭
		VkCompareOp depthCompareOp = VK_COMPARE_OP_NEVER; ///< 深度比较操作，默认永不通过
		VkBool32 depthBoundsTestEnable = VK_FALSE; ///< 是否启用深度边界测试，默认关闭
		VkBool32 stencilTestEnable = VK_FALSE;     ///< 是否启用模板测试，默认关闭
	};

	/**
	 * @brief 管线动态状态配置结构体
	 */
	struct PipelineDynamicState {
		std::vector<VkDynamicState> dynamicStates; ///< 动态状态数组
	};

	/**
	 * @brief 管线完整配置结构体，包含所有图形管线状态
	 */
	struct PipelineConfig {
		PipelineVertexInputState vertexInputState;                    ///< 顶点输入状态
		PipelineInputAssemblyState inputAssemblyState;                ///< 输入装配状态
		PipelineRasterizationState rasterizationState;                ///< 光栅化状态
		PipelineMultisampleState multisampleState;                    ///< 多重采样状态
		PipelineDepthStencilState depthStencilState;                  ///< 深度/模板测试状态
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState{ ///< 颜色混合附件状态
			.blendEnable = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
					  | VK_COLOR_COMPONENT_G_BIT
					  | VK_COLOR_COMPONENT_B_BIT
					  | VK_COLOR_COMPONENT_A_BIT
		};
		PipelineDynamicState dynamicState;                            ///< 动态状态
	};

	/**
	 * @brief Vulkan管线布局类，用于创建和管理着色器模块及管线布局对象
	 */
	class AdVKPipelineLayout {
	public:
		/**
		 * @brief 构造函数，初始化管线布局对象
		 * @param device 指向Vulkan设备对象的指针
		 * @param vertexShaderFile 顶点着色器文件路径
		 * @param fragShaderFile 片段着色器文件路径
		 * @param shaderLayout 着色器布局信息（可选）
		 */
		AdVKPipelineLayout(AdVKDevice* device, const std::string& vertexShaderFile, const std::string& fragShaderFile, const ShaderLayout& shaderLayout = {});

		/**
		 * @brief 析构函数，释放资源
		 */
		~AdVKPipelineLayout();

		/**
		 * @brief 获取管线布局句柄
		 * @return 返回Vulkan管线布局句柄
		 */
		VkPipelineLayout GetHandle() const { return mHandle; }

		/**
		 * @brief 获取顶点着色器模块句柄
		 * @return 返回Vulkan着色器模块句柄
		 */
		VkShaderModule GetVertexShaderModule() const { return mVertexShaderModule; }

		/**
		 * @brief 获取片段着色器模块句柄
		 * @return 返回Vulkan着色器模块句柄
		 */
		VkShaderModule GetFragShaderModule() const { return mFragShaderModule; }

	private:
		/**
		 * @brief 创建着色器模块
		 * @param filePath 着色器文件路径
		 * @param outShaderModule 输出参数，指向创建的着色器模块句柄
		 * @return Vulkan结果代码
		 */
		VkResult CreateShaderModule(const std::string& filePath, VkShaderModule* outShaderModule);

		VkPipelineLayout mHandle = VK_NULL_HANDLE;           ///< 管线布局句柄
		VkShaderModule mVertexShaderModule = VK_NULL_HANDLE; ///< 顶点着色器模块句柄
		VkShaderModule mFragShaderModule = VK_NULL_HANDLE;   ///< 片段着色器模块句柄
		AdVKDevice* mDevice;                                 ///< 指向Vulkan设备对象的指针
	};

	/**
	 * @brief Vulkan图形管线类，用于构建和管理图形管线对象
	 */
	class AdVKPipeline {
	public:
		/**
		 * @brief 构造函数，初始化管线对象
		 * @param device 指向Vulkan设备对象的指针
		 * @param renderPass 指向渲染通道对象的指针
		 * @param pipelineLayout 指向管线布局对象的指针
		 */
		AdVKPipeline(AdVKDevice* device, AdVKRenderPass* renderPass, AdVKPipelineLayout* pipelineLayout);

		/**
		 * @brief 析构函数，释放资源
		 */
		~AdVKPipeline();

		/**
		 * @brief 创建图形管线对象
		 */
		void Create();

		/**
		 * @brief 绑定当前管线到命令缓冲区
		 * @param cmdBuffer 命令缓冲区句柄
		 */
		void Bind(VkCommandBuffer cmdBuffer);

		/**
		 * @brief 设置顶点输入状态
		 * @param vertexBindings 顶点绑定描述数组
		 * @param vertexAttrs 顶点属性描述数组
		 * @return 返回当前对象指针，支持链式调用
		 */
		AdVKPipeline* SetVertexInputState(const std::vector<VkVertexInputBindingDescription>& vertexBindings, const std::vector<VkVertexInputAttributeDescription>& vertexAttrs);

		/**
		 * @brief 设置输入装配状态
		 * @param topology 图元拓扑类型
		 * @param primitiveRestartEnable 是否启用图元重启功能，默认关闭
		 * @return 返回当前对象指针，支持链式调用
		 */
		AdVKPipeline* SetInputAssemblyState(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable = VK_FALSE);

		/**
		 * @brief 设置光栅化状态
		 * @param rasterizationState 光栅化状态配置结构体
		 * @return 返回当前对象指针，支持链式调用
		 */
		AdVKPipeline* SetRasterizationState(const PipelineRasterizationState& rasterizationState);

		/**
		 * @brief 设置多重采样状态
		 * @param samples 采样数
		 * @param sampleShadingEnable 是否启用样本着色
		 * @param minSampleShading 最小样本着色比例，默认为0
		 * @return 返回当前对象指针，支持链式调用
		 */
		AdVKPipeline* SetMultisampleState(VkSampleCountFlagBits samples, VkBool32 sampleShadingEnable, float minSampleShading = 0.f);

		/**
		 * @brief 设置深度和模板测试状态
		 * @param depthStencilState 深度/模板测试状态配置结构体
		 * @return 返回当前对象指针，支持链式调用
		 */
		AdVKPipeline* SetDepthStencilState(const PipelineDepthStencilState& depthStencilState);

		/**
		 * @brief 设置颜色混合附件状态
		 * @param blendEnable 是否启用混合
		 * @param srcColorBlendFactor 源颜色混合因子，默认为VK_BLEND_FACTOR_ONE
		 * @param dstColorBlendFactor 目标颜色混合因子，默认为VK_BLEND_FACTOR_ZERO
		 * @param colorBlendOp 颜色混合操作，默认为VK_BLEND_OP_ADD
		 * @param srcAlphaBlendFactor 源Alpha混合因子，默认为VK_BLEND_FACTOR_ONE
		 * @param dstAlphaBlendFactor 目标Alpha混合因子，默认为VK_BLEND_FACTOR_ZERO
		 * @param alphaBlendOp Alpha混合操作，默认为VK_BLEND_OP_ADD
		 * @return 返回当前对象指针，支持链式调用
		 */
		AdVKPipeline* SetColorBlendAttachmentState(VkBool32 blendEnable,
			VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE, VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, VkBlendOp colorBlendOp = VK_BLEND_OP_ADD,
			VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD);

		/**
		 * @brief 设置动态状态
		 * @param dynamicStates 动态状态数组
		 * @return 返回当前对象指针，支持链式调用
		 */
		AdVKPipeline* SetDynamicState(const std::vector<VkDynamicState>& dynamicStates);

		/**
		 * @brief 启用Alpha混合
		 * @return 返回当前对象指针，支持链式调用
		 */
		AdVKPipeline* EnableAlphaBlend();

		/**
		 * @brief 启用深度测试
		 * @return 返回当前对象指针，支持链式调用
		 */
		AdVKPipeline* EnableDepthTest();

		/**
		 * @brief 获取管线句柄
		 * @return 返回Vulkan管线句柄
		 */
		VkPipeline GetHandle() const { return mHandle; }

	private:
		VkPipeline mHandle = VK_NULL_HANDLE;         ///< 管线句柄
		AdVKDevice* mDevice;                         ///< 指向Vulkan设备对象的指针
		AdVKRenderPass* mRenderPass;                 ///< 指向渲染通道对象的指针
		AdVKPipelineLayout* mPipelineLayout;         ///< 指向管线布局对象的指针

		PipelineConfig mPipelineConfig;              ///< 管线配置信息
	};
}

#endif