#include "ECS/System/AdPBRMaterialSystem.h"

#include "AdFileUtil.h"
#include "AdApplication.h"
#include "Graphic/AdVKPipeline.h"
#include "Graphic/AdVKDescriptorSet.h"
#include "Graphic/AdVKImageView.h"
#include "Graphic/AdVKFrameBuffer.h"

#include "Render/AdRenderTarget.h"

#include "ECS/Component/AdTransformComponent.h"

namespace WuDu {
	//��ʼ�����ղ���ϵͳ
	void AdPBRMaterialSystem::OnInit(AdVKRenderPass* renderPass) {
		AdVKDevice* device = GetDevice();

		// ����֡UBO�����������֣����ڴ���ͶӰ������ͼ�����ÿ֡���������
		{
			const std::vector<VkDescriptorSetLayoutBinding> bindings = {
				{
					.binding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				}
			};
			mFrameUboDescSetLayout = std::make_shared<AdVKDescriptorSetLayout>(device, bindings);
		}
		{
			const std::vector<VkDescriptorSetLayoutBinding> bindings = {
				{
					.binding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				}
			};
			mMaterialParamDescSetLayout = std::make_shared<AdVKDescriptorSetLayout>(device, bindings);
		}

		// ����������Դ�����������֣����ڴ��������������ͼ����ͼ
		{
			const std::vector<VkDescriptorSetLayoutBinding> bindings = {
				{
					.binding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
				},
				{
					.binding = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	}
			};
			mMaterialResourceDescSetLayout = std::make_shared<AdVKDescriptorSetLayout>(device, bindings);
		}

		//�������ͳ���: ���ڴ���ģ�ͱ任����
		VkPushConstantRange modelPC = {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof(ModelPC)
		};

		//������ɫ�����ֲ��������߲���
		ShaderLayout shaderLayout = {
			.descriptorSetLayouts = { mFrameUboDescSetLayout->GetHandle(), mMaterialParamDescSetLayout->GetHandle(), mMaterialResourceDescSetLayout->GetHandle() },
			.pushConstants = { modelPC }
		};
		mPipelineLayout = std::make_shared<AdVKPipelineLayout>(
			device,
			AD_RES_SHADER_DIR"",
			AD_RES_SHADER_DIR"",
			shaderLayout
		);

		//���ö��������ʽ
		std::vector<VkVertexInputBindingDescription> vertexBindings = {
			{
				.binding = 0,
				.stride = sizeof(AdVertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			}
		};

		std::vector<VkVertexInputAttributeDescription> vertexAttrs = {
			{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(AdVertex,Position)
			},
			{
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(AdVertex,TexCoord)
			},
			{
				.location = 2,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(AdVertex,Normal)
			},
			{
				.location = 3,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(AdVertex,Tangent)
			},
			{
				.location = 4,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset  = offsetof(AdVertex,Bitangent)
			}
		};

		//����ͼ�ι��߲�������ز���
		mPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mPipelineLayout.get());
		mPipeline->SetVertexInputState(vertexBindings, vertexAttrs);
		mPipeline->EnableDepthTest();
		mPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR });
		mPipeline->SetMultisampleState(VK_SAMPLE_COUNT_4_BIT, VK_FALSE);
		mPipeline->SetSubPassIndex(0);
		mPipeline->Create();

		//�����������ز�����֡UBO��������
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = 1
			}
		};
		mDescriptorPool = std::make_shared<AdVKDescriptorPool>(device, 1, poolSizes);
		mFrameUboDescSet = mDescriptorPool->AllocateDescriptorSet(mFrameUboDescSetLayout.get(), 1)[0];
		mFrameUboBuffer = std::make_shared<AdVKBuffer>(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(FrameUbo), nullptr, true);

		//��ʼ��������������
		ReCreateMaterialDescPool(NUM_MATERIAL_BATCH);

		//����Ĭ������
		//std::unique_ptr<RGBAColor> whitePixel = std::make_unique<RGBAColor>(255, 255, 255, 255);
		RGBAColor pixel { 255, 255, 255, 255 };
		mDefaultTexture = std::make_shared<AdTexture>(1, 1, & pixel);

		//����Ĭ�ϲ�����

		mDefaultSampler = std::make_shared<AdSampler>();
	}

	void AdPBRMaterialSystem::OnRender(VkCommandBuffer cmdBuffer, AdRenderTarget* renderTarget) {
		Adscene* scene = GetScene();
		if(!scene){
			return;
		}
		entt::registry& reg = scene->GetEcsRegistry();


		//��ȡ���п���������޹��մ�ְ�����ʵ��ͼ
		auto view = reg.view<AdTransformComponent, AdPBRMaterialComponent>
		if(std::distance(view.begin(), view.end()) == 0){
			return;
		}

		//��ͼ�ι��߲������ӿںͲü�����
		mPipeline->Bind(cmdBuffer);
		AdVKFrameBuffer* frameBuffer = renderTarget->GetFrameBuffer();
		VkViewport viewport = {
			.x = 0,
			.y = 0,
			.width = static_cast<float>(frameBuffer->GetWidth()),
			.height = static_cast<float>(frameBuffer->GetHeight()),
			.minDepth = 0.f,
			.maxDepth = 1.f
		};
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = {
			.offset = { 0 , 0 },
			.extent = { frameBuffer->GetWidth(), frameBuffer->GetHeight() }
		};
		vkCmdSetScissor(cmdBuffer,0,1,&scissor);

		//����֡UBO��������
		UpdateFrameUboDescSet(renderTarget);

		//����Ƿ���Ҫ���´���������������
		bool bShouldForceUpdateMaterial = false;
		uin32_t materialCount = AdMaterialFactory::GetInstance()->GetMaterialSize<AdPBRMaterial>();
		if(materialCount > mLastDescriptorSetCount){
			ReCreateMaterialDescPool(materialCount);
			bShouldForceUpdateMaterial = true;
		}

		//��������ʵ�岢��Ⱦ
		std::vector<bool> updateFlags(materialCount);
		view.each([this, &updateFlags, &bShouldForceUpdateMaterial, &cmdBuffer](AdTransformComponent& transComp,AdPBRMaterialComponent& materialComp){
			for(const auto& entry : materialComp.GetMeshMaterials()){
				AdPBRMaterial* material = entry.first;
			}
		})
	}
}