#ifndef AD_VKPIPELINE_H
#define AD_VKPIPELINE_H

#include "AdVKCommon.h"

namespace ade {
	class AdVKDevice;
	class AdVKRenderPass;

	/**
	 * @brief ������ɫ���Ĳ�����Ϣ���������������ϲ��ֺ����ͳ�����Χ
	 */
	struct ShaderLayout {
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts; ///< ����������������
		std::vector<VkPushConstantRange> pushConstants;          ///< ���ͳ�����Χ����
	};

	/**
	 * @brief ���߶�������״̬���ýṹ��
	 */
	struct PipelineVertexInputState {
		std::vector<VkVertexInputBindingDescription> vertexBindings;     ///< �������������
		std::vector<VkVertexInputAttributeDescription> vertexAttributes; ///< ����������������
	};

	/**
	 * @brief ��������װ��״̬���ýṹ��
	 */
	struct PipelineInputAssemblyState {
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; ///< ͼԪ�������ͣ�Ĭ��Ϊ�������б�
		VkBool32 primitiveRestartEnable = VK_FALSE;                         ///< �Ƿ�����ͼԪ�������ܣ�Ĭ�Ϲر�
	};

	/**
	 * @brief ���߹�դ��״̬���ýṹ��
	 */
	struct PipelineRasterizationState {
		VkBool32 depthClampEnable = VK_FALSE;                ///< �Ƿ�������Ƚ�ȡ��Ĭ�Ϲر�
		VkBool32 rasterizerDiscardEnable = VK_FALSE;         ///< �Ƿ�����դ�������Ĭ�ϲ�����
		VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;    ///< ��������ģʽ��Ĭ�����
		VkCullModeFlags cullMode = VK_CULL_MODE_NONE;        ///< ���޳�ģʽ��Ĭ�ϲ��޳�
		VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; ///< ǰ������Ĭ����ʱ��
		VkBool32 depthBiasEnable = VK_FALSE;                 ///< �Ƿ��������ƫ�ƣ�Ĭ�Ϲر�
		float depthBiasConstantFactor = 0;                   ///< ���ƫ�Ƴ�������
		float depthBiasClamp = 0;                            ///< ���ƫ��ǯλֵ
		float depthBiasSlopeFactor = 0;                      ///< ���ƫ��б������
		float lineWidth = 1.f;                               ///< �߿�Ĭ��Ϊ1.0
	};

	/**
	 * @brief ���߶��ز���״̬���ýṹ��
	 */
	struct PipelineMultisampleState {
		VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; ///< ��դ����������Ĭ��Ϊ1������
		VkBool32 sampleShadingEnable = VK_FALSE;                            ///< �Ƿ�����������ɫ��Ĭ�Ϲر�
		float minSampleShading = 0.2f;                                      ///< ��С������ɫ����
	};

	/**
	 * @brief ������Ⱥ�ģ�����״̬���ýṹ��
	 */
	struct PipelineDepthStencilState {
		VkBool32 depthTestEnable = VK_FALSE;       ///< �Ƿ�������Ȳ��ԣ�Ĭ�Ϲر�
		VkBool32 depthWriteEnable = VK_FALSE;      ///< �Ƿ��������д�룬Ĭ�Ϲر�
		VkCompareOp depthCompareOp = VK_COMPARE_OP_NEVER; ///< ��ȱȽϲ�����Ĭ������ͨ��
		VkBool32 depthBoundsTestEnable = VK_FALSE; ///< �Ƿ�������ȱ߽���ԣ�Ĭ�Ϲر�
		VkBool32 stencilTestEnable = VK_FALSE;     ///< �Ƿ�����ģ����ԣ�Ĭ�Ϲر�
	};

	/**
	 * @brief ���߶�̬״̬���ýṹ��
	 */
	struct PipelineDynamicState {
		std::vector<VkDynamicState> dynamicStates; ///< ��̬״̬����
	};

	/**
	 * @brief �����������ýṹ�壬��������ͼ�ι���״̬
	 */
	struct PipelineConfig {
		PipelineVertexInputState vertexInputState;                    ///< ��������״̬
		PipelineInputAssemblyState inputAssemblyState;                ///< ����װ��״̬
		PipelineRasterizationState rasterizationState;                ///< ��դ��״̬
		PipelineMultisampleState multisampleState;                    ///< ���ز���״̬
		PipelineDepthStencilState depthStencilState;                  ///< ���/ģ�����״̬
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState{ ///< ��ɫ��ϸ���״̬
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
		PipelineDynamicState dynamicState;                            ///< ��̬״̬
	};

	/**
	 * @brief Vulkan���߲����࣬���ڴ����͹�����ɫ��ģ�鼰���߲��ֶ���
	 */
	class AdVKPipelineLayout {
	public:
		/**
		 * @brief ���캯������ʼ�����߲��ֶ���
		 * @param device ָ��Vulkan�豸�����ָ��
		 * @param vertexShaderFile ������ɫ���ļ�·��
		 * @param fragShaderFile Ƭ����ɫ���ļ�·��
		 * @param shaderLayout ��ɫ��������Ϣ����ѡ��
		 */
		AdVKPipelineLayout(AdVKDevice* device, const std::string& vertexShaderFile, const std::string& fragShaderFile, const ShaderLayout& shaderLayout = {});

		/**
		 * @brief �����������ͷ���Դ
		 */
		~AdVKPipelineLayout();

		/**
		 * @brief ��ȡ���߲��־��
		 * @return ����Vulkan���߲��־��
		 */
		VkPipelineLayout GetHandle() const { return mHandle; }

		/**
		 * @brief ��ȡ������ɫ��ģ����
		 * @return ����Vulkan��ɫ��ģ����
		 */
		VkShaderModule GetVertexShaderModule() const { return mVertexShaderModule; }

		/**
		 * @brief ��ȡƬ����ɫ��ģ����
		 * @return ����Vulkan��ɫ��ģ����
		 */
		VkShaderModule GetFragShaderModule() const { return mFragShaderModule; }

	private:
		/**
		 * @brief ������ɫ��ģ��
		 * @param filePath ��ɫ���ļ�·��
		 * @param outShaderModule ���������ָ�򴴽�����ɫ��ģ����
		 * @return Vulkan�������
		 */
		VkResult CreateShaderModule(const std::string& filePath, VkShaderModule* outShaderModule);

		VkPipelineLayout mHandle = VK_NULL_HANDLE;           ///< ���߲��־��
		VkShaderModule mVertexShaderModule = VK_NULL_HANDLE; ///< ������ɫ��ģ����
		VkShaderModule mFragShaderModule = VK_NULL_HANDLE;   ///< Ƭ����ɫ��ģ����
		AdVKDevice* mDevice;                                 ///< ָ��Vulkan�豸�����ָ��
	};

	/**
	 * @brief Vulkanͼ�ι����࣬���ڹ����͹���ͼ�ι��߶���
	 */
	class AdVKPipeline {
	public:
		/**
		 * @brief ���캯������ʼ�����߶���
		 * @param device ָ��Vulkan�豸�����ָ��
		 * @param renderPass ָ����Ⱦͨ�������ָ��
		 * @param pipelineLayout ָ����߲��ֶ����ָ��
		 */
		AdVKPipeline(AdVKDevice* device, AdVKRenderPass* renderPass, AdVKPipelineLayout* pipelineLayout);

		/**
		 * @brief �����������ͷ���Դ
		 */
		~AdVKPipeline();

		/**
		 * @brief ����ͼ�ι��߶���
		 */
		void Create();

		/**
		 * @brief �󶨵�ǰ���ߵ��������
		 * @param cmdBuffer ����������
		 */
		void Bind(VkCommandBuffer cmdBuffer);

		/**
		 * @brief ���ö�������״̬
		 * @param vertexBindings �������������
		 * @param vertexAttrs ����������������
		 * @return ���ص�ǰ����ָ�룬֧����ʽ����
		 */
		AdVKPipeline* SetVertexInputState(const std::vector<VkVertexInputBindingDescription>& vertexBindings, const std::vector<VkVertexInputAttributeDescription>& vertexAttrs);

		/**
		 * @brief ��������װ��״̬
		 * @param topology ͼԪ��������
		 * @param primitiveRestartEnable �Ƿ�����ͼԪ�������ܣ�Ĭ�Ϲر�
		 * @return ���ص�ǰ����ָ�룬֧����ʽ����
		 */
		AdVKPipeline* SetInputAssemblyState(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable = VK_FALSE);

		/**
		 * @brief ���ù�դ��״̬
		 * @param rasterizationState ��դ��״̬���ýṹ��
		 * @return ���ص�ǰ����ָ�룬֧����ʽ����
		 */
		AdVKPipeline* SetRasterizationState(const PipelineRasterizationState& rasterizationState);

		/**
		 * @brief ���ö��ز���״̬
		 * @param samples ������
		 * @param sampleShadingEnable �Ƿ�����������ɫ
		 * @param minSampleShading ��С������ɫ������Ĭ��Ϊ0
		 * @return ���ص�ǰ����ָ�룬֧����ʽ����
		 */
		AdVKPipeline* SetMultisampleState(VkSampleCountFlagBits samples, VkBool32 sampleShadingEnable, float minSampleShading = 0.f);

		/**
		 * @brief ������Ⱥ�ģ�����״̬
		 * @param depthStencilState ���/ģ�����״̬���ýṹ��
		 * @return ���ص�ǰ����ָ�룬֧����ʽ����
		 */
		AdVKPipeline* SetDepthStencilState(const PipelineDepthStencilState& depthStencilState);

		/**
		 * @brief ������ɫ��ϸ���״̬
		 * @param blendEnable �Ƿ����û��
		 * @param srcColorBlendFactor Դ��ɫ������ӣ�Ĭ��ΪVK_BLEND_FACTOR_ONE
		 * @param dstColorBlendFactor Ŀ����ɫ������ӣ�Ĭ��ΪVK_BLEND_FACTOR_ZERO
		 * @param colorBlendOp ��ɫ��ϲ�����Ĭ��ΪVK_BLEND_OP_ADD
		 * @param srcAlphaBlendFactor ԴAlpha������ӣ�Ĭ��ΪVK_BLEND_FACTOR_ONE
		 * @param dstAlphaBlendFactor Ŀ��Alpha������ӣ�Ĭ��ΪVK_BLEND_FACTOR_ZERO
		 * @param alphaBlendOp Alpha��ϲ�����Ĭ��ΪVK_BLEND_OP_ADD
		 * @return ���ص�ǰ����ָ�룬֧����ʽ����
		 */
		AdVKPipeline* SetColorBlendAttachmentState(VkBool32 blendEnable,
			VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE, VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, VkBlendOp colorBlendOp = VK_BLEND_OP_ADD,
			VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD);

		/**
		 * @brief ���ö�̬״̬
		 * @param dynamicStates ��̬״̬����
		 * @return ���ص�ǰ����ָ�룬֧����ʽ����
		 */
		AdVKPipeline* SetDynamicState(const std::vector<VkDynamicState>& dynamicStates);

		/**
		 * @brief ����Alpha���
		 * @return ���ص�ǰ����ָ�룬֧����ʽ����
		 */
		AdVKPipeline* EnableAlphaBlend();

		/**
		 * @brief ������Ȳ���
		 * @return ���ص�ǰ����ָ�룬֧����ʽ����
		 */
		AdVKPipeline* EnableDepthTest();

		/**
		 * @brief ��ȡ���߾��
		 * @return ����Vulkan���߾��
		 */
		VkPipeline GetHandle() const { return mHandle; }

	private:
		VkPipeline mHandle = VK_NULL_HANDLE;         ///< ���߾��
		AdVKDevice* mDevice;                         ///< ָ��Vulkan�豸�����ָ��
		AdVKRenderPass* mRenderPass;                 ///< ָ����Ⱦͨ�������ָ��
		AdVKPipelineLayout* mPipelineLayout;         ///< ָ����߲��ֶ����ָ��

		PipelineConfig mPipelineConfig;              ///< ����������Ϣ
	};
}

#endif