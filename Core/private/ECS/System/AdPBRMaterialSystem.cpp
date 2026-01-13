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
	//初始化PBR材质渲染系统
	void AdPBRMaterialSystem::OnInit(AdVKRenderPass* renderPass) {
		AdVKDevice* device = GetDevice();

		// 创建帧UBO描述符布局，用于存储投影矩阵和纹理等每一帧更新的数据
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

		// 创建材质资源描述符布局，用于存储材质参数和纹理等
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

		//创建推送常量范围: 用于存储模型矩阵数据
		VkPushConstantRange modelPC = {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof(ModelPC)
		};

		//创建着色器资源布局和推送常量
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

		//设置顶点输入格式
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

		//创建图形渲染管线和相关设置
		mPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mPipelineLayout.get());
		mPipeline->SetVertexInputState(vertexBindings, vertexAttrs);
		mPipeline->EnableDepthTest();
		mPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR });
		mPipeline->SetMultisampleState(VK_SAMPLE_COUNT_4_BIT, VK_FALSE);
		mPipeline->SetSubPassIndex(0);
		mPipeline->Create();

		//创建渲染管线的每帧UBO相关资源
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = 1
			}
		};
		mDescriptorPool = std::make_shared<AdVKDescriptorPool>(device, 1, poolSizes);
		mFrameUboDescSet = mDescriptorPool->AllocateDescriptorSet(mFrameUboDescSetLayout.get(), 1)[0];
		mFrameUboBuffer = std::make_shared<AdVKBuffer>(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(FrameUbo), nullptr, true);

		//初始化材质描述符池
		ReCreateMaterialDescPool(NUM_MATERIAL_BATCH);

		//创建默认纹理
		//std::unique_ptr<RGBAColor> whitePixel = std::make_unique<RGBAColor>(255, 255, 255, 255);
		RGBAColor pixel { 255, 255, 255, 255 };
		mDefaultTexture = std::make_shared<AdTexture>(1, 1, & pixel);

		//创建默认采样器

		mDefaultSampler = std::make_shared<AdSampler>();
	}

	void AdPBRMaterialSystem::OnRender(VkCommandBuffer cmdBuffer, AdRenderTarget* renderTarget) {
		AdScene* scene = GetScene();
		if(!scene){
			return;
		}
		entt::registry& reg = scene->GetEcsRegistry();


		//获取所有有变换和PBR材质组件的实体
		auto view = reg.view<AdTransformComponent, AdPBRMaterialComponent>();
		if(std::distance(view.begin(), view.end()) == 0){
			return;
		}

		//将图形渲染管线绑定到命令缓冲区
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

		//更新每帧UBO描述符集
		UpdateFrameUboDescSet(renderTarget);

		//检查是否需要重建材质描述符池
		bool bShouldForceUpdateMaterial = false;
		uint32_t materialCount = AdMaterialFactory::GetInstance()->GetMaterialSize<AdPBRMaterial>();
		if(materialCount > mLastDescriptorSetCount){
			ReCreateMaterialDescPool(materialCount);
			bShouldForceUpdateMaterial = true;
		}

		//按材质实体批次渲染
		std::vector<bool> updateFlags(materialCount);
		view.each([this, &updateFlags, &bShouldForceUpdateMaterial, &cmdBuffer](AdTransformComponent& transComp,AdPBRMaterialComponent& materialComp){
			for(const auto& entry : materialComp.GetMeshMaterials()){
				AdPBRMaterial* material = entry.first;
			}
		});
	}
}