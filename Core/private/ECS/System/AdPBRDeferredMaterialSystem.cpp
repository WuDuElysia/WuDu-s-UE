#include "ECS/System/AdPBRDeferredMaterialSystem.h"

#include "AdFileUtil.h"
#include "AdApplication.h"
#include "Graphic/AdVKPipeline.h"
#include "Graphic/AdVKDescriptorSet.h"
#include "Graphic/AdVKImageView.h"
#include "Graphic/AdVKFrameBuffer.h"
#include "Render/AdRenderTarget.h"
#include "ECS/Component/AdTransformComponent.h"
#include "ECS/Component/Light/AdDirectionalLightComponent.h"
#include "ECS/Component/Light/AdPointLightComponent.h"
#include "ECS/Component/Light/AdSpotLightComponent.h"

namespace WuDu {
    //初始化PBR材质渲染系统
    void AdPBRDeferredMaterialSystem::OnInit(AdVKRenderPass* renderPass) {
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
		//创建材质参数描述符布局
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
		//创建光照UBO描述符布局
		{
			const std::vector<VkDescriptorSetLayoutBinding> bindings = {
				{
					.binding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				}
			};
			mLightUboDescSetLayout = std::make_shared<AdVKDescriptorSetLayout>(device, bindings);
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
				},
				{
					.binding = 2,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
				},
				{
					.binding = 3,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
				},
				{
					.binding = 4,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
				}
			};
			mMaterialResourceDescSetLayout = std::make_shared<AdVKDescriptorSetLayout>(device, bindings);
		}
        //创建GBuffer描述符布局
        //0 -> basecolor+Alpha
        //1 -> normal+emissive
        //2 -> metallic+roughness+ao
        //3 -> depth
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
                },
                {
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
                },
                {
                    .binding = 3,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
                }
            };
            mGBufferDescSetLayout = std::make_shared<AdVKDescriptorSetLayout>(device, bindings);
        }
        //创建IBL资源描述符布局
        //0 -> environment map
        //1 -> irradiance map
        //2 -> prefilter map
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
                },
                {
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
                }
            };
            mIBLResourceDescSetLayout = std::make_shared<AdVKDescriptorSetLayout>(device, bindings);
        }
        //创建光照描述符布局
        //0 -> dirlight final
        //1 -> IBL final
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
            mLightingDescSetLayout = std::make_shared<AdVKDescriptorSetLayout>(device, bindings);
        }
        //创建后处理描述符布局 
        {
            const std::vector<VkDescriptorSetLayoutBinding> bindings = {
                {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
                }
            };
            mPostProcessDescSetLayout = std::make_shared<AdVKDescriptorSetLayout>(device, bindings);
        }

		//创建推送常量范围: 用于存储模型矩阵数据
		VkPushConstantRange modelPC = {
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof(ModelPC)
		};

        // GBuffer 管线布局
        ShaderLayout shaderLayout = {
            .descriptorSetLayouts = {
                mFrameUboDescSetLayout->GetHandle(),
                mMaterialParamDescSetLayout->GetHandle(),
                mMaterialResourceDescSetLayout->GetHandle()
            },
            .pushConstants = { modelPC }
        };
        mGBufferPipelineLayout = std::make_shared<AdVKPipelineLayout>(
            device,
            AD_RES_SHADER_DIR"PBR_Deferred_GBuffer.vert",
            AD_RES_SHADER_DIR"PBR_Deferred_GBuffer.frag",
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

        //Gbuffer管线布局
        mGBufferPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mGBufferPipelineLayout.get());
        mGBufferPipeline->SetVertexInputState(vertexBindings, vertexAttrs);
        mGBufferPipeline->EnableDepthTest();
        mGBufferPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR });
        mGBufferPipeline->SetSubPassIndex(0);
        mGBufferPipeline->Create();

        //直接光照管线布局
        ShaderLayout lightingShaderLayout = {
            .descriptorSetLayouts = {
                mFrameUboDescSetLayout->GetHandle(),
                mGBufferDescSetLayout->GetHandle(),
                mLightUboDescSetLayout->GetHandle(),
            },
            .pushConstants = {}
        };
        mDirectLightingPipelineLayout = std::make_shared<AdVKPipelineLayout>(
            device,
            AD_RES_SHADER_DIR"PBR_Deferred_Lighting.vert",
            AD_RES_SHADER_DIR"PBR_Deferred_Lighting.frag",
            lightingShaderLayout
        );

        mDirectLightingPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mDirectLightingPipelineLayout.get());
        mDirectLightingPipeline->SetVertexInputState({}, {});
        mDirectLightingPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR });
        mDirectLightingPipeline->SetSubPassIndex(1);
        mDirectLightingPipeline->Create();

        //IBL管线布局
        ShaderLayout iblshaderLayout = {
            .descriptorSetLayouts = {
                mFrameUboDescSetLayout->GetHandle(),
                mGBufferDescSetLayout->GetHandle(),
                mIBLResourceDescSetLayout->GetHandle(),
            },
            .pushConstants = {}
        };
        mIBLPipelineLayout = std::make_shared<AdVKPipelineLayout>(
            device,
            AD_RES_SHADER_DIR"PBR_Deferred_IBL.vert",
            AD_RES_SHADER_DIR"PBR_Deferred_IBL.frag",
            iblshaderLayout
        );

        mIBLPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mIBLPipelineLayout.get());
        mIBLPipeline->SetVertexInputState({}, {});
        mIBLPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR });
        mIBLPipeline->SetSubPassIndex(2);
        mIBLPipeline->Create();

        //Merge管线布局
        ShaderLayout mergeShaderLayout = {
            .descriptorSetLayouts = {
                mGBufferDescSetLayout->GetHandle(),
                mLightingDescSetLayout->GetHandle(),
            },
            .pushConstants = {}
        };

        mMergePipelineLayout = std::make_shared<AdVKPipelineLayout>(
            device,
            AD_RES_SHADER_DIR"PBR_Deferred_Merge.vert",
            AD_RES_SHADER_DIR"PBR_Deferred_Merge.frag",
            mergeShaderLayout
        );

        mMergePipeline = std::make_shared<AdVKPipeline>(device, renderPass, mMergePipelineLayout.get());
        mMergePipeline->SetVertexInputState({}, {});
        mMergePipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR });
        mMergePipeline->SetSubPassIndex(3);
        mMergePipeline->Create();

        //PostProcess管线布局
        ShaderLayout postProcessShaderLayout = {
            .descriptorSetLayouts = {
                mFrameUboDescSetLayout->GetHandle(),
                mLightingDescSetLayout->GetHandle(),
                mPostProcessDescSetLayout->GetHandle()
            },
            .pushConstants = {}
        };

        mPostProcessPipelineLayout = std::make_shared<AdVKPipelineLayout>(
            device,
            AD_RES_SHADER_DIR"PBR_Deferred_PostProcess.vert",
            AD_RES_SHADER_DIR"PBR_Deferred_PostProcess.frag",
            postProcessShaderLayout
        );

        mPostProcessPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mPostProcessPipelineLayout.get());
        mPostProcessPipeline->SetVertexInputState({}, {});
        mPostProcessPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR });
        mPostProcessPipeline->SetSubPassIndex(4);
        mPostProcessPipeline->Create();

        //创建渲染管线的每帧UBO相关资源
        std::vector<VkDescriptorPoolSize> poolSizes = {
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1
            }
        };
        mDescriptorPool = std::make_shared<AdVKDescriptorPool>(device, 1, poolSizes);
        mFrameUboDescSet = mDescriptorPool->AllocateDescriptorSet(mFrameUboDescSetLayout.get(), 1)[0];
        mFrameUbobuffer = std::make_shared<AdVKBuffer>(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(FrameUbo),nullptr,true);
        mLightUboBuffer = std::make_shared<AdVKBuffer>(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(LightUbo), nullptr, true);
        mPostProcessBuffer = std::make_shared<AdVKBuffer>(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(PostProcessUbo), nullptr, true);

        //初始化材质描述符池
        ReCreateMaterialDescPool(NUM_MATERIAL_BATCH);

        //创建默认纹理和采样器
        RGBAColor pixel { 255, 255, 255, 255 };
        mDefaultTexture = std::make_shared<AdTexture>(1,1,&pixel);
        mDefaultSampler = std::make_shared<AdSampler>();

    }

    void AdPBRDeferredMaterialSystem::OnRender(VkCommandBuffer cmdBuffer, AdRenderTarget* renderTarget) {

        AdScene* scene = GetScene();
        if(!scene) return;
        
        entt::registry& reg = scene->GetEcsRegistry();

		// 更新帧 UBO
		UpdateFrameUboDescSet(renderTarget);
		
		// 更新光源 UBO
		UpdateLightUboDescSet();
		
		// 更新 GBuffer 描述符集（将 GBuffer 附件绑定到描述符集）
		UpdateGbufferDescSet();
		
		// 更新 IBL 资源描述符集
		UpdateIBLResourceDescSet();
		
		// 更新光照结果描述符集（将直接光照和 IBL 结果绑定到描述符集）
		UpdateLightingDescSet();
		
		// 更新后处理 UBO
		UpdatePostProcessDescSet();

		//鉴擦是否需要重建材质描述符集
		bool bShouldForceUpdateMaterial = false;
		uint32_t materialCount = AdMaterialFactory::GetInstance()->GetMaterialSize<AdPBRMaterial>();

        //获取所有变换和PBR材质组件
        auto view = reg.view<AdTransformComponent, AdPBRMaterialComponent>();
        if(std::distance(view.begin(),view.end()) == 0) return;

		//将图形管线绑定到命令缓冲区

		//subpass 0: GBuffer
		{
			mGBufferPipeline->Bind(cmdBuffer);
			AdVKFrameBuffer* frameBuffer = renderTarget->GetFrameBuffer();
			VkViewport viewport = {
				.x = 0.f,
				.y = 0.f,
				.width = static_cast<float>(frameBuffer->GetWidth()),
				.height = static_cast<float>(frameBuffer->GetHeight()),
				.minDepth = 0.f,
				.maxDepth = 1.f
			};
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

			VkRect2D scissor = {
				.offset = {0,0},
				.extent = {frameBuffer->GetWidth(), frameBuffer->GetHeight()}
			};
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

			//绑定帧UBO描述符集
			vkCmdBindDescriptorSets(cmdBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,
				mGBufferPipelineLayout->GetHandle(),0,1,&mFrameUboDescSet,0,nullptr);

			//遍历所有实体，渲染具有PBR材质的实体
			std::vector<bool> updateFlags(materialCount);
			view.each([this, &updateFlags, &bShouldForceUpdateMaterial, &cmdBuffer](AdTransformComponent& transComp, AdPBRMaterialComponent& materialComp) {
				for(const auto& entry : materialComp.GetMeshMaterials()) {
					AdPBRMaterial* material = entry.first;
					if(!material || material->GetIndex() < 0) {
						LOG_W("default material or error material");
						continue;
					}

					//查找当前材质对应的索引
					uint32_t materialIndex = material->GetIndex();
					VkDescriptorSet paramsDescSet = mMaterialDescSets[materialIndex];
					VkDescriptorSet resourceDescSet = mMaterialResourceDescSets[materialIndex];

                    //绑定材质参数描述符集和材质资源描述符集
                    if(!updateFlags[materialIndex] || bShouldForceUpdateMaterial) {
                        UpdateMaterialParamsDescSet(paramsDescSet, material);
                        UpdateMaterialResourceDescSet(resourceDescSet, material);
                        updateFlags[materialIndex] = true;
                    }

                    //绑定材质描述符集
                    VkDescriptorSet descriptorSets[] = {
                        mFrameUboDescSet,
                        paramsDescSet,
                        resourceDescSet
                    };

                    vkCmdBindDescriptorSets(
                        cmdBuffer,
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        mGBufferPipelineLayout->GetHandle(),
                        0,
                        ARRAY_SIZE(descriptorSets),
                        descriptorSets,
                        0,
                        nullptr
                    );
                    
                    ModelPC pc = {
                        transComp.GetTransform()
                    };
                    vkCmdPushContants(
                        cmdBuffer,
                        mGBufferPipelineLayout->GetHandle(),
                        VK_SHADER_STAGE_VERTEX_BIT,
                        0,
                        sizeof(ModelPC),
                        &pc
                    );

                    //绘制网格
                    for(const auto& meshIndex : entry.second){
                        materialComp.GetMesh(meshIndex)->Draw(cmdBuffer);
                    }
				}
					
			});

		}

        //subpass 1: Direct Lighting
        {
            mDirectLightingPipeline->Bind(cmdBuffer);

            //设置视口和裁剪区域
            AdVKFrameBuffer* frameBuffer = renderTarget->GetFrameBuffer();
            VkViewport viewport = {
                .x = 0.f,
                .y = 0.f,
                .width = static_cast<float>(frameBuffer->GetWidth()),
                .height = static_cast<float>(frameBuffer->GetHeight()),
                .minDepth = 0.f,
                .maxDepth = 1.f
            };
            
        }
    }

}