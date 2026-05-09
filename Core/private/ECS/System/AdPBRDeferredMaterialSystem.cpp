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
        mGBufferPipeline->SetColorAttachmentCount(3);  // subpass 0 有 3 个颜色附件（GBuffer 0/1/2）
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
        // 需要分配的描述符集：
        //   mFrameUboDescSet (1 UBO)
        //   mLightUboDescSet (1 UBO)
        //   mGBufferDescSet (4 COMBINED_IMAGE_SAMPLER)
        //   mIBLResourceDescSet (3 COMBINED_IMAGE_SAMPLER)
        //   mLightingDescSet (2 COMBINED_IMAGE_SAMPLER)
        //   mPostProcessDescSet (1 COMBINED_IMAGE_SAMPLER)
        // 总计: 6 个描述符集, 2 个 UBO, 10 个 COMBINED_IMAGE_SAMPLER
        std::vector<VkDescriptorPoolSize> poolSizes = {
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 2
            },
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 10
            }
        };
        mDescriptorPool = std::make_shared<AdVKDescriptorPool>(device, 6, poolSizes);
        mFrameUboDescSet = mDescriptorPool->AllocateDescriptorSet(mFrameUboDescSetLayout.get(), 1)[0];
        mLightUboDescSet = mDescriptorPool->AllocateDescriptorSet(mLightUboDescSetLayout.get(), 1)[0];
        mGBufferDescSet = mDescriptorPool->AllocateDescriptorSet(mGBufferDescSetLayout.get(), 1)[0];
        mIBLResourceDescSet = mDescriptorPool->AllocateDescriptorSet(mIBLResourceDescSetLayout.get(), 1)[0];
        mLightingDescSet = mDescriptorPool->AllocateDescriptorSet(mLightingDescSetLayout.get(), 1)[0];
        mPostProcessDescSet = mDescriptorPool->AllocateDescriptorSet(mPostProcessDescSetLayout.get(), 1)[0];
        mFrameUboBuffer = std::make_shared<AdVKBuffer>(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(DeferredFrameUbo),nullptr,true);
        mLightUboBuffer = std::make_shared<AdVKBuffer>(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(LightingUbo), nullptr, true);
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

        // 保存当前渲染目标引用，供无参数的描述符集更新方法使用
        mCurrentRenderTarget = renderTarget;
        
        entt::registry& reg = scene->GetEcsRegistry();

		// 更新帧 UBO
		UpdateFrameUboDescSet(renderTarget);
		
		// 更新光源 UBO
		UpdateLightUboDescSet();
		
		// 更新 GBuffer 描述符集（将 GBuffer 附件绑定到描述符集）
		UpdateGBufferDescSet();
		
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
                    
                    glm::mat4 modelMat = transComp.GetTransform();
                    glm::mat3 normalMat = -glm::transpose(glm::inverse(glm::mat3(modelMat)));
                    ModelPC pc = {
                        modelMat,
                        glm::vec4(normalMat[0], 0.0f),
                        glm::vec4(normalMat[1], 0.0f),
                        glm::vec4(normalMat[2], 0.0f)
                    };
                    vkCmdPushConstants(
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

        vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

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
            vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

            VkRect2D scissor = {
                .offset = {0, 0},
                .extent = {frameBuffer->GetWidth(), frameBuffer->GetHeight()}
            };
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

            //绑定描述符集
            VkDescriptorSet lightDescSets[] = {
                mFrameUboDescSet,
                mGBufferDescSet,
                mLightUboDescSet
            };
            vkCmdBindDescriptorSets(
                cmdBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                mDirectLightingPipelineLayout->GetHandle(),
                0,
                ARRAY_SIZE(lightDescSets),
                lightDescSets,
                0,
                nullptr
            );

            //绘制全屏四边形
            vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
        }

        vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

        //subpass 2: IBL
        {
            mIBLPipeline->Bind(cmdBuffer);

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
            vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

            VkRect2D scissor = {
                .offset = {0, 0},
                .extent = {frameBuffer->GetWidth(), frameBuffer->GetHeight()}
            };
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

            //绑定描述符集
            VkDescriptorSet iblDescSets[] = {
                mFrameUboDescSet,
                mGBufferDescSet,
                mIBLResourceDescSet
            };
            vkCmdBindDescriptorSets(
                cmdBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                mIBLPipelineLayout->GetHandle(),
                0,
                ARRAY_SIZE(iblDescSets),
                iblDescSets,
                0,
                nullptr
            );

            //绘制全屏四边形
            vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
        }

        vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

        //subpass 3: Merge
        {
            mMergePipeline->Bind(cmdBuffer);

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
            vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

            VkRect2D scissor = {
                .offset = {0, 0},
                .extent = {frameBuffer->GetWidth(), frameBuffer->GetHeight()}
            };
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

            //绑定描述符集
            VkDescriptorSet mergeDescSets[] = {
                mGBufferDescSet,
                mLightingDescSet
            };
            vkCmdBindDescriptorSets(
                cmdBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                mMergePipelineLayout->GetHandle(),
                0,
                ARRAY_SIZE(mergeDescSets),
                mergeDescSets,
                0,
                nullptr
            );

            //绘制全屏四边形
            vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
        }

        vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

        //subpass 4: PostProcess
        {
            mPostProcessPipeline->Bind(cmdBuffer);

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
            vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

            VkRect2D scissor = {
                .offset = {0, 0},
                .extent = {frameBuffer->GetWidth(), frameBuffer->GetHeight()}
            };
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

            //绑定描述符集
            VkDescriptorSet postProcessDescSets[] = {
                mFrameUboDescSet,
                mLightingDescSet,
                mPostProcessDescSet
            };
            vkCmdBindDescriptorSets(
                cmdBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                mPostProcessPipelineLayout->GetHandle(),
                0,
                ARRAY_SIZE(postProcessDescSets),
                postProcessDescSets,
                0,
                nullptr
            );

            //绘制全屏四边形
            vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
        }   
        mLastDescriptorSetCount = materialCount;
    }

    //销毁pbr材质系统资源
    void AdPBRDeferredMaterialSystem::OnDestroy() {
        mGBufferPipeline.reset();
        mGBufferPipelineLayout.reset();
        mDirectLightingPipeline.reset();
        mDirectLightingPipelineLayout.reset();
        mIBLPipeline.reset();
        mIBLPipelineLayout.reset();
        mMergePipeline.reset();
        mMergePipelineLayout.reset();
        mPostProcessPipeline.reset();
        mPostProcessPipelineLayout.reset();

    }

    //重新创建材质描述符池
    void AdPBRDeferredMaterialSystem::ReCreateMaterialDescPool(uint32_t materialCount){
        AdVKDevice* device = GetDevice();

        //计算新的描述符集数量
        uint32_t newDescriptorSetCount = mLastDescriptorSetCount;
        if(mLastDescriptorSetCount == 0){
            newDescriptorSetCount = NUM_MATERIAL_BATCH;
        }

        while(newDescriptorSetCount < materialCount){
            newDescriptorSetCount *= 2;
        }

        if(newDescriptorSetCount > NUM_MATERIAL_BATCH_MAX){
            LOG_E("Descriptor Set max count is : {0}, but request : {1}", NUM_MATERIAL_BATCH_MAX, newDescriptorSetCount);
			return;
        }

        LOG_W("{0}: {1} -> {2} S.", __FUNCTION__, mLastDescriptorSetCount, newDescriptorSetCount);

        //销毁旧的描述符池和相关资源
        std::vector<VkDescriptorPoolSize> poosize = {
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = newDescriptorSetCount
            },
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = newDescriptorSetCount * 5
            },
        };

        mMaterialDescriptorPool = std::make_shared<WuDu::AdVKDescriptorPool>(device, newDescriptorSetCount * 2, poosize);

        //分配新的描述符集
		mMaterialDescSets = mMaterialDescriptorPool->AllocateDescriptorSet(mMaterialParamDescSetLayout.get(), newDescriptorSetCount);
		mMaterialResourceDescSets = mMaterialDescriptorPool->AllocateDescriptorSet(mMaterialResourceDescSetLayout.get(), newDescriptorSetCount);
		assert(mMaterialDescSets.size() == newDescriptorSetCount && "Failed to AllocateDescriptorSet");
		assert(mMaterialResourceDescSets.size() == newDescriptorSetCount && "Failed to AllocateDescriptorSet");

		//创建新的材质缓冲区
		uint32_t diffCount = newDescriptorSetCount - mLastDescriptorSetCount;
		for (int i = 0; i < diffCount; i++) {
			auto materialBuffer = std::make_shared<AdVKBuffer>(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(PBRMaterialUbo), nullptr, true);
			mMaterialBuffers.push_back(materialBuffer);
		}
		LOG_W("{0}: {1} -> {2} E.", __FUNCTION__, mLastDescriptorSetCount, newDescriptorSetCount);
		mLastDescriptorSetCount = newDescriptorSetCount;
    }

    //更新帧UBO的描述符集（使用DeferredFrameUbo，包含逆矩阵和相机位置）
    void AdPBRDeferredMaterialSystem::UpdateFrameUboDescSet(AdRenderTarget* renderTarget){
        AdApplication* app = GetApp();
		AdVKDevice* device = GetDevice();

		AdVKFrameBuffer* frameBuffer = renderTarget->GetFrameBuffer();
		glm::ivec2 resolution = { frameBuffer->GetWidth(), frameBuffer->GetHeight() };

		glm::mat4 projMat = GetProjMat(renderTarget);
		glm::mat4 viewMat = GetViewMat(renderTarget);

		//构造延迟渲染帧UBO数据（含逆矩阵和相机位置）
		DeferredFrameUbo frameUbo = {
			.projMat = projMat,
			.viewMat = viewMat,
			.invProjMat = glm::inverse(projMat),
			.invViewMat = glm::inverse(viewMat),
			.camPos = GetCameraPosition(renderTarget),
			._pad0 = 0.0f,
			.resolution = resolution,
			.frameId = static_cast<uint32_t>(app->GetFrameIndex()),
			.time = app->GetStartTimeSecond()
		};

		//写入缓冲区并更新描述符集
		mFrameUboBuffer->WriteData(&frameUbo);
		VkDescriptorBufferInfo bufferInfo = DescriptorSetWriter::BuildBufferInfo(mFrameUboBuffer->GetHandle(), 0, sizeof(DeferredFrameUbo));
		VkWriteDescriptorSet bufferWrite = DescriptorSetWriter::WriteBuffer(mFrameUboDescSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
		DescriptorSetWriter::UpdateDescriptorSets(device->GetHandle(), { bufferWrite });
    }

    //更新材质参数描述符集
    void AdPBRDeferredMaterialSystem::UpdateMaterialParamsDescSet(VkDescriptorSet descSet, AdPBRMaterial* material){
        AdVKDevice* device = GetDevice();

        AdVKBuffer* materialBuffer = mMaterialBuffers[material->GetIndex()].get();


        //获取材质参数
        PBRMaterialUbo params = material->GetParams();

        //更新参数
        const TextureView* baseColorTexture = material->GetTextureView(PBR_MAT_BASE_COLOR);
        if(baseColorTexture){
            AdMaterial::UpdateTextureParams(baseColorTexture, &params.baseColorTextureParam);
        }

        const TextureView* normalTexture = material->GetTextureView(PBR_MAT_NORMAL);
        if(normalTexture){
            AdMaterial::UpdateTextureParams(normalTexture, &params.normalTextureParam);
        }

        const TextureView* metallicRoughnessTexture = material->GetTextureView(PBR_MAT_METALLIC_ROUGHNESS);
        if(metallicRoughnessTexture){
            AdMaterial::UpdateTextureParams(metallicRoughnessTexture, &params.metallicRoughnessTextureParam);
        }

        const TextureView* aoTexture = material->GetTextureView(PBR_MAT_AO);
        if(aoTexture){
            AdMaterial::UpdateTextureParams(aoTexture, &params.aoTextureParam);
        }

        const TextureView* emissiveTexture = material->GetTextureView(PBR_MAT_EMISSIVE);
        if(emissiveTexture){
            AdMaterial::UpdateTextureParams(emissiveTexture, &params.emissiveTextureParam);
        }

        //写入缓冲区并更新描述符集
        materialBuffer->WriteData(&params);
        VkDescriptorBufferInfo bufferInfo = DescriptorSetWriter::BuildBufferInfo(materialBuffer->GetHandle(), 0, sizeof(params));
        VkWriteDescriptorSet bufferWrite = DescriptorSetWriter::WriteBuffer(descSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
		DescriptorSetWriter::UpdateDescriptorSets(device->GetHandle(), { bufferWrite });
    }

    //更新材质资源描述符集，传递纹理采样器和图像视图
    void AdPBRDeferredMaterialSystem::UpdateMaterialResourceDescSet(VkDescriptorSet descSet, AdPBRMaterial* material){
        AdVKDevice* device = GetDevice();

        //获取纹理资源
        const TextureView* baseColorTexture = material->GetTextureView(PBR_MAT_BASE_COLOR);
        const TextureView* normalTexture = material->GetTextureView(PBR_MAT_NORMAL);
        const TextureView* metallicRoughnessTexture = material->GetTextureView(PBR_MAT_METALLIC_ROUGHNESS);
        const TextureView* aoTexture = material->GetTextureView(PBR_MAT_AO);
        const TextureView* emissiveTexture = material->GetTextureView(PBR_MAT_EMISSIVE);

        //设置默认纹理资源
        TextureView defaultView;
        defaultView.texture = mDefaultTexture.get();
        defaultView.sampler = mDefaultSampler.get();

        //如果没有设置纹理,使用默认纹理
        if(!baseColorTexture || !baseColorTexture->sampler || !baseColorTexture->texture || !baseColorTexture->texture->GetImageView()) baseColorTexture = &defaultView;
        if(!normalTexture || !normalTexture->sampler || !normalTexture->texture || !normalTexture->texture->GetImageView()) normalTexture = &defaultView;
        if(!metallicRoughnessTexture || !metallicRoughnessTexture->sampler || !metallicRoughnessTexture->texture || !metallicRoughnessTexture->texture->GetImageView()) metallicRoughnessTexture = &defaultView;
        if(!aoTexture || !aoTexture->sampler || !aoTexture->texture || !aoTexture->texture->GetImageView()) aoTexture = &defaultView;
        if(!emissiveTexture || !emissiveTexture->sampler || !emissiveTexture->texture || !emissiveTexture->texture->GetImageView()) emissiveTexture = &defaultView;

        //构建图像信息
        VkDescriptorImageInfo baseColorImageInfo = DescriptorSetWriter::BuildImageInfo(
            baseColorTexture->sampler->GetHandle(),
            baseColorTexture->texture->GetImageView()->GetHandle()
        );
        VkDescriptorImageInfo normalImageInfo = DescriptorSetWriter::BuildImageInfo(
            normalTexture->sampler->GetHandle(),
            normalTexture->texture->GetImageView()->GetHandle()
        );
        VkDescriptorImageInfo metallicRoughnessImageInfo = DescriptorSetWriter::BuildImageInfo(
            metallicRoughnessTexture->sampler->GetHandle(),
            metallicRoughnessTexture->texture->GetImageView()->GetHandle()
        );
        VkDescriptorImageInfo aoImageInfo = DescriptorSetWriter::BuildImageInfo(
            aoTexture->sampler->GetHandle(),
            aoTexture->texture->GetImageView()->GetHandle()
        );
        VkDescriptorImageInfo emissiveImageInfo = DescriptorSetWriter::BuildImageInfo(
            emissiveTexture->sampler->GetHandle(),
            emissiveTexture->texture->GetImageView()->GetHandle()
        );

        //更新描述符集
        VkWriteDescriptorSet baseColorWrite = DescriptorSetWriter::WriteImage(
            descSet, PBR_MAT_BASE_COLOR, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &baseColorImageInfo
        );
        VkWriteDescriptorSet normalWrite = DescriptorSetWriter::WriteImage(
            descSet, PBR_MAT_NORMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &normalImageInfo
        );
        VkWriteDescriptorSet metallicRoughnessWrite = DescriptorSetWriter::WriteImage(
            descSet, PBR_MAT_METALLIC_ROUGHNESS, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &metallicRoughnessImageInfo
        );
        VkWriteDescriptorSet aoWrite = DescriptorSetWriter::WriteImage(
            descSet, PBR_MAT_AO, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &aoImageInfo
        );
        VkWriteDescriptorSet emissiveWrite = DescriptorSetWriter::WriteImage(
            descSet, PBR_MAT_EMISSIVE, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &emissiveImageInfo
        );

        DescriptorSetWriter::UpdateDescriptorSets(device->GetHandle(), { baseColorWrite, normalWrite, metallicRoughnessWrite, aoWrite, emissiveWrite });
    }

    //更新GBuffer描述符集，将GBuffer附件的ImageView绑定到mGBufferDescSet
    void AdPBRDeferredMaterialSystem::UpdateGBufferDescSet(){
        AdVKDevice* device = GetDevice();
        AdVKFrameBuffer* frameBuffer = mCurrentRenderTarget->GetFrameBuffer();

        // 获取GBuffer附件的ImageView（索引0-3）
        VkImageView baseColorView = frameBuffer->GetAttachmentImageView(0);
        VkImageView normalEmissiveView = frameBuffer->GetAttachmentImageView(1);
        VkImageView metallicRoughnessAOView = frameBuffer->GetAttachmentImageView(2);
        VkImageView depthView = frameBuffer->GetAttachmentImageView(3);

        VkSampler sampler = mDefaultSampler->GetHandle();

        // 构建图像信息
        VkDescriptorImageInfo baseColorImageInfo = DescriptorSetWriter::BuildImageInfo(sampler, baseColorView);
        VkDescriptorImageInfo normalEmissiveImageInfo = DescriptorSetWriter::BuildImageInfo(sampler, normalEmissiveView);
        VkDescriptorImageInfo metallicRoughnessAOImageInfo = DescriptorSetWriter::BuildImageInfo(sampler, metallicRoughnessAOView);
        VkDescriptorImageInfo depthImageInfo = DescriptorSetWriter::BuildImageInfo(sampler, depthView);

        // 写入4个binding到mGBufferDescSet
        VkWriteDescriptorSet baseColorWrite = DescriptorSetWriter::WriteImage(
            mGBufferDescSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &baseColorImageInfo
        );
        VkWriteDescriptorSet normalEmissiveWrite = DescriptorSetWriter::WriteImage(
            mGBufferDescSet, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &normalEmissiveImageInfo
        );
        VkWriteDescriptorSet metallicRoughnessAOWrite = DescriptorSetWriter::WriteImage(
            mGBufferDescSet, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &metallicRoughnessAOImageInfo
        );
        VkWriteDescriptorSet depthWrite = DescriptorSetWriter::WriteImage(
            mGBufferDescSet, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &depthImageInfo
        );

        DescriptorSetWriter::UpdateDescriptorSets(device->GetHandle(), { baseColorWrite, normalEmissiveWrite, metallicRoughnessAOWrite, depthWrite });
    }

    //更新光照UBO描述符集（与前向渲染一致）
    void AdPBRDeferredMaterialSystem::UpdateLightUboDescSet(){
        AdVKDevice* device = GetDevice();
        auto& registry = GetScene()->GetEcsRegistry();

        //收集所有光源组件
        std::vector<LightUbo> lightUbos;

        //遍历方向光
        auto directionalLightView = registry.view<AdDirectionalLightComponent>();
        directionalLightView.each([&lightUbos](AdDirectionalLightComponent& dirLightComp) {
            lightUbos.push_back(dirLightComp.GetLightUbo());
        });

        //遍历点光 - 在系统层组装LightUbo，从TransformComponent获取位置
        auto pointLightView = registry.view<AdPointLightComponent, AdTransformComponent>();
        pointLightView.each([&lightUbos](AdPointLightComponent& pointLightComp, AdTransformComponent& transComp) {
            LightUbo ubo{};
            ubo.position = glm::vec4(transComp.GetWorldPosition(), 1.0f);
            ubo.directionAndRange = glm::vec4(0.0f, 0.0f, 0.0f, pointLightComp.GetRange());
            ubo.colorAndIntensity = glm::vec4(pointLightComp.GetColor(), pointLightComp.GetIntensity());
            ubo.attenuationConstant = pointLightComp.GetAttenuationConstant();
            ubo.attenuationLinear = pointLightComp.GetAttenuationLinear();
            ubo.attenuationQuadratic = pointLightComp.GetAttenuationQuadratic();
            ubo.type = static_cast<uint32_t>(LightType::LIGHT_TYPE_POINT);
            ubo.enabled = pointLightComp.IsEnabled() ? 1 : 0;
            lightUbos.push_back(ubo);
        });

        //遍历聚光灯 - 在系统层组装LightUbo，从TransformComponent获取位置和方向
        auto spotLightView = registry.view<AdSpotLightComponent, AdTransformComponent>();
        spotLightView.each([&lightUbos](AdSpotLightComponent& spotLightComp, AdTransformComponent& transComp) {
            LightUbo ubo{};
            ubo.position = glm::vec4(transComp.GetWorldPosition(), 1.0f);
            ubo.directionAndRange = glm::vec4(transComp.GetForwardDirection(), spotLightComp.GetRange());
            ubo.colorAndIntensity = glm::vec4(spotLightComp.GetColor(), spotLightComp.GetIntensity());
            ubo.spotInnerCutoff = spotLightComp.GetSpotInnerCutoffCos();
            ubo.spotOuterCutoff = spotLightComp.GetSpotOuterCutoffCos();
            ubo.attenuationConstant = spotLightComp.GetAttenuationConstant();
            ubo.attenuationLinear = spotLightComp.GetAttenuationLinear();
            ubo.attenuationQuadratic = spotLightComp.GetAttenuationQuadratic();
            ubo.type = static_cast<uint32_t>(LightType::LIGHT_TYPE_SPOT);
            ubo.enabled = spotLightComp.IsEnabled() ? 1 : 0;
            lightUbos.push_back(ubo);
        });

        //更新光照UBO数据
        LightingUbo lightingUbo{};
        lightingUbo.ambientColorAndIntensity = glm::vec4(mAmbientColor, mAmbientIntensity);
        lightingUbo.numLights = static_cast<uint32_t>(lightUbos.size());

        //复制光源数据到光照UBO
        for (size_t i = 0; i < lightUbos.size() && i < MAX_LIGHTS; ++i) {
            lightingUbo.lights[i] = lightUbos[i];
        }

        //写入光照UBO缓冲区
        mLightUboBuffer->WriteData(&lightingUbo);

        //更新描述符集
        VkDescriptorBufferInfo bufferInfo = DescriptorSetWriter::BuildBufferInfo(mLightUboBuffer->GetHandle(), 0, sizeof(LightingUbo));
        VkWriteDescriptorSet bufferWrite = DescriptorSetWriter::WriteBuffer(mLightUboDescSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
        DescriptorSetWriter::UpdateDescriptorSets(device->GetHandle(), { bufferWrite });
    }

    //更新IBL资源描述符集，将IBL纹理（或默认纹理）绑定到mIBLResourceDescSet
    void AdPBRDeferredMaterialSystem::UpdateIBLResourceDescSet(){
        AdVKDevice* device = GetDevice();

        VkSampler sampler = mDefaultSampler->GetHandle();
        VkImageView irradianceView;
        VkImageView prefilterView;
        VkImageView brdfLUTView;

        // 如果IBL资源已加载且有效，使用IBL纹理；否则使用默认纹理占位
        if (mIBLLoaded && mIrradianceMap && mIrradianceMap->GetImageView()
            && mPrefilterMap && mPrefilterMap->GetImageView()
            && mBrdfLUT && mBrdfLUT->GetImageView()) {
            irradianceView = mIrradianceMap->GetImageView()->GetHandle();
            prefilterView = mPrefilterMap->GetImageView()->GetHandle();
            brdfLUTView = mBrdfLUT->GetImageView()->GetHandle();
        } else {
            VkImageView defaultImageView = mDefaultTexture->GetImageView()->GetHandle();
            irradianceView = defaultImageView;
            prefilterView = defaultImageView;
            brdfLUTView = defaultImageView;
        }

        // 构建3个binding的图像信息（irradianceMap, prefilterMap, brdfLUT）
        VkDescriptorImageInfo irradianceImageInfo = DescriptorSetWriter::BuildImageInfo(sampler, irradianceView);
        VkDescriptorImageInfo prefilterImageInfo = DescriptorSetWriter::BuildImageInfo(sampler, prefilterView);
        VkDescriptorImageInfo brdfLUTImageInfo = DescriptorSetWriter::BuildImageInfo(sampler, brdfLUTView);

        // 写入3个binding到mIBLResourceDescSet
        VkWriteDescriptorSet irradianceWrite = DescriptorSetWriter::WriteImage(
            mIBLResourceDescSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &irradianceImageInfo
        );
        VkWriteDescriptorSet prefilterWrite = DescriptorSetWriter::WriteImage(
            mIBLResourceDescSet, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &prefilterImageInfo
        );
        VkWriteDescriptorSet brdfLUTWrite = DescriptorSetWriter::WriteImage(
            mIBLResourceDescSet, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &brdfLUTImageInfo
        );

        DescriptorSetWriter::UpdateDescriptorSets(device->GetHandle(), { irradianceWrite, prefilterWrite, brdfLUTWrite });
    }

    //更新光照结果描述符集，将直接光照累积附件和IBL累积附件绑定到mLightingDescSet
    void AdPBRDeferredMaterialSystem::UpdateLightingDescSet(){
        AdVKDevice* device = GetDevice();
        AdVKFrameBuffer* frameBuffer = mCurrentRenderTarget->GetFrameBuffer();

        // 获取直接光照累积附件（附件4）和IBL累积附件（附件5）的ImageView
        VkImageView directLightingView = frameBuffer->GetAttachmentImageView(4);
        VkImageView iblView = frameBuffer->GetAttachmentImageView(5);

        VkSampler sampler = mDefaultSampler->GetHandle();

        // 构建图像信息
        VkDescriptorImageInfo directLightingImageInfo = DescriptorSetWriter::BuildImageInfo(sampler, directLightingView);
        VkDescriptorImageInfo iblImageInfo = DescriptorSetWriter::BuildImageInfo(sampler, iblView);

        // 写入2个binding到mLightingDescSet
        VkWriteDescriptorSet directLightingWrite = DescriptorSetWriter::WriteImage(
            mLightingDescSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &directLightingImageInfo
        );
        VkWriteDescriptorSet iblWrite = DescriptorSetWriter::WriteImage(
            mLightingDescSet, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &iblImageInfo
        );

        DescriptorSetWriter::UpdateDescriptorSets(device->GetHandle(), { directLightingWrite, iblWrite });
    }

    //更新后处理描述符集，将合并光照附件（附件6）的ImageView绑定到mPostProcessDescSet
    void AdPBRDeferredMaterialSystem::UpdatePostProcessDescSet(){
        AdVKDevice* device = GetDevice();
        AdVKFrameBuffer* frameBuffer = mCurrentRenderTarget->GetFrameBuffer();

        // 获取合并光照附件（附件6）的ImageView
        VkImageView mergedLightingView = frameBuffer->GetAttachmentImageView(6);

        VkSampler sampler = mDefaultSampler->GetHandle();

        // 构建图像信息
        VkDescriptorImageInfo mergedLightingImageInfo = DescriptorSetWriter::BuildImageInfo(sampler, mergedLightingView);

        // 写入binding 0到mPostProcessDescSet
        VkWriteDescriptorSet mergedLightingWrite = DescriptorSetWriter::WriteImage(
            mPostProcessDescSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &mergedLightingImageInfo
        );

        DescriptorSetWriter::UpdateDescriptorSets(device->GetHandle(), { mergedLightingWrite });
    }

    //加载IBL资源（HDR环境贴图 → 辐照度贴图 + 预滤波贴图 + BRDF LUT）
    void AdPBRDeferredMaterialSystem::LoadIBLResources(const std::string& hdrPath) {
        // TODO: Implement IBL resource generation from HDR environment map
        // For now, log a message and set the flag
        LOG_W("LoadIBLResources: IBL precomputation not yet implemented, using default textures. Path: {}", hdrPath);
        mIBLLoaded = false;
    }

}