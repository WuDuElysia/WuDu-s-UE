#include "ECS/System/AdUnlitMaterialSystem.h"

#include "AdFileUtil.h"
#include "AdApplication.h"
#include "Graphic/AdVKPipeline.h"
#include "Graphic/AdVKDescriptorSet.h"
#include "Graphic/AdVKImageView.h"
#include "Graphic/AdVKFrameBuffer.h"

#include "Render/AdRenderTarget.h"

#include "ECS/Component/AdTransformComponent.h"

namespace WuDu {
        /**
 * @brief 初始化无光照材质系统，创建渲染管线、描述符布局、描述符池等资源。
 *
 * @param renderPass Vulkan 渲染通道对象，用于管线创建。
 */
        void AdUnlitMaterialSystem::OnInit(AdVKRenderPass* renderPass) {
                AdVKDevice* device = GetDevice();

                // 创建帧UBO描述符集布局：用于传递投影矩阵、视图矩阵等每帧不变的数据
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

                // 创建材质参数描述符集布局：用于传递材质参数（如颜色、纹理参数等）
                {
                        const std::vector<VkDescriptorSetLayoutBinding> bindings = {
                            {
                                .binding = 0,
                                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                .descriptorCount = 1,
                                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
                            }
                        };
                        mMaterialParamDescSetLayout = std::make_shared<AdVKDescriptorSetLayout>(device, bindings);
                }

                // 创建材质资源描述符集布局：用于传递纹理采样器和图像视图
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

                // 定义推送常量：用于传递模型变换矩阵
                VkPushConstantRange modelPC = {
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .offset = 0,
                    .size = sizeof(ModelPC)
                };

                // 构建着色器布局并创建管线布局
                ShaderLayout shaderLayout = {
                    .descriptorSetLayouts = { mFrameUboDescSetLayout->GetHandle(), mMaterialParamDescSetLayout->GetHandle(), mMaterialResourceDescSetLayout->GetHandle() },
                    .pushConstants = { modelPC }
                };
                mPipelineLayout = std::make_shared<AdVKPipelineLayout>(device,
                        AD_RES_SHADER_DIR"03_unlit_material.vert",
                        AD_RES_SHADER_DIR"03_unlit_material.frag",
                        shaderLayout);

                // 配置顶点输入格式：位置、纹理坐标、法线
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
                        .offset = offsetof(AdVertex, pos)
                    },
                    {
                        .location = 1,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32_SFLOAT,
                        .offset = offsetof(AdVertex, tex)
                    },
                    {
                        .location = 2,
                        .binding = 0,
                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                        .offset = offsetof(AdVertex, nor)
                    }
                };

                // 创建图形管线并设置相关状态
                mPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mPipelineLayout.get());
                mPipeline->SetVertexInputState(vertexBindings, vertexAttrs);
                mPipeline->EnableDepthTest();
                mPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
                mPipeline->SetMultisampleState(VK_SAMPLE_COUNT_4_BIT, VK_FALSE);
                mPipeline->SetSubPassIndex(0);
                mPipeline->Create();

                // 创建描述符池并分配帧UBO描述符集
                std::vector<VkDescriptorPoolSize> poolSizes = {
                    {
                        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .descriptorCount = 1
                    }
                };
                mDescriptorPool = std::make_shared<AdVKDescriptorPool>(device, 1, poolSizes);
                mFrameUboDescSet = mDescriptorPool->AllocateDescriptorSet(mFrameUboDescSetLayout.get(), 1)[0];
                mFrameUboBuffer = std::make_shared<WuDu::AdVKBuffer>(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(FrameUbo), nullptr, true);

                // 初始化材质描述符池
                ReCreateMaterialDescPool(NUM_MATERIAL_BATCH);

                // 创建默认纹理（白色像素）
                RGBAColor* whitePixel = new RGBAColor{255,255,255,255};
                mDefaultTexture = std::make_shared<AdTexture>(1, 1, whitePixel);

                // 创建默认采样器
               
                mDefaultSampler = std::make_shared<AdSampler>();
        }

        /**
         * @brief 执行无光照材质系统的渲染逻辑。
         *
         * @param cmdBuffer Vulkan 命令缓冲区，用于记录渲染命令。
         * @param renderTarget 渲染目标对象，包含帧缓冲等信息。
         */
        void AdUnlitMaterialSystem::OnRender(VkCommandBuffer cmdBuffer, AdRenderTarget* renderTarget) {
                AdScene* scene = GetScene();
                if (!scene) {
                        return;
                }
                entt::registry& reg = scene->GetEcsRegistry();

                // 获取具有变换组件和无光照材质组件的实体视图
                auto view = reg.view<AdTransformComponent, AdUnlitMaterialComponent>();
                if (std::distance(view.begin(), view.end()) == 0) {
                        return;
                }

                // 绑定图形管线并设置视口和裁剪区域
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
                    .offset = { 0, 0 },
                    .extent = { frameBuffer->GetWidth(), frameBuffer->GetHeight() }
                };
                vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

                // 更新帧UBO描述符集
                UpdateFrameUboDescSet(renderTarget);

                // 检查是否需要重新创建材质描述符池
                bool bShouldForceUpdateMaterial = false;
                uint32_t materialCount = AdMaterialFactory::GetInstance()->GetMaterialSize<AdUnlitMaterial>();
                if (materialCount > mLastDescriptorSetCount) {
                        ReCreateMaterialDescPool(materialCount);
                        bShouldForceUpdateMaterial = true;
                }

                // 遍历所有实体并渲染
                std::vector<bool> updateFlags(materialCount);
                view.each([this, &updateFlags, &bShouldForceUpdateMaterial, &cmdBuffer](AdTransformComponent& transComp, AdUnlitMaterialComponent& materialComp) {
                        for (const auto& entry : materialComp.GetMeshMaterials()) {
                                AdUnlitMaterial* material = entry.first;
                                if (!material || material->GetIndex() < 0) {
                                        LOG_W("TODO: default material or error material ?");
                                        continue;
                                }

                                uint32_t materialIndex = material->GetIndex();
                                VkDescriptorSet paramsDescSet = mMaterialDescSets[materialIndex];
                                VkDescriptorSet resourceDescSet = mMaterialResourceDescSets[materialIndex];

                                // 更新材质参数和资源描述符集
                                // 修复：确保首次使用时一定更新描述符集
                                if (!updateFlags[materialIndex] || bShouldForceUpdateMaterial) {
                                        // 移除 ShouldFlush 检查，或者确保新材质的 flush 标志正确设置
                                        UpdateMaterialParamsDescSet(paramsDescSet, material);
                                        UpdateMaterialResourceDescSet(resourceDescSet, material);
                                        updateFlags[materialIndex] = true;
                                }

                                // 绑定描述符集并推送模型变换矩阵
                                VkDescriptorSet descriptorSets[] = { mFrameUboDescSet, paramsDescSet, resourceDescSet };
                                vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout->GetHandle(),
                                        0, ARRAY_SIZE(descriptorSets), descriptorSets, 0, nullptr);

                                ModelPC pc = { transComp.GetTransform() };
                                vkCmdPushConstants(cmdBuffer, mPipelineLayout->GetHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

                                // 绘制网格
                                for (const auto& meshIndex : entry.second) {
                                        materialComp.GetMesh(meshIndex)->Draw(cmdBuffer);
                                }
                        }
                        });
        }

        /**
         * @brief 销毁无光照材质系统资源。
         */
        void AdUnlitMaterialSystem::OnDestroy() {
                mPipeline.reset();
                mPipelineLayout.reset();
        }

        /**
         * @brief 重新创建材质描述符池，以适应更多材质的需求。
         *
         * @param materialCount 当前需要支持的材质数量。
         */
        void AdUnlitMaterialSystem::ReCreateMaterialDescPool(uint32_t materialCount) {
                AdVKDevice* device = GetDevice();

                // 计算新的描述符集数量
                uint32_t newDescriptorSetCount = mLastDescriptorSetCount;
                if (mLastDescriptorSetCount == 0) {
                        newDescriptorSetCount = NUM_MATERIAL_BATCH;
                }

                while (newDescriptorSetCount < materialCount) {
                        newDescriptorSetCount *= 2;
                }

                if (newDescriptorSetCount > NUM_MATERIAL_BATCH_MAX) {
                        LOG_E("Descriptor Set max count is : {0}, but request : {1}", NUM_MATERIAL_BATCH_MAX, newDescriptorSetCount);
                        return;
                }

                LOG_W("{0}: {1} -> {2} S.", __FUNCTION__, mLastDescriptorSetCount, newDescriptorSetCount);

                // 销毁旧的描述符池和相关资源
                mMaterialDescSets.clear();
                mMaterialResourceDescSets.clear();
                if (mMaterialDescriptorPool) {
                        mMaterialDescriptorPool.reset();
                }

                // 创建新的描述符池
                std::vector<VkDescriptorPoolSize> poolSizes = {
                    {
                        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .descriptorCount = newDescriptorSetCount
                    },
                    {
                        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .descriptorCount = newDescriptorSetCount * 2
                    }
                };
                mMaterialDescriptorPool = std::make_shared<WuDu::AdVKDescriptorPool>(device, newDescriptorSetCount * 2, poolSizes);

                // 分配新的描述符集
                mMaterialDescSets = mMaterialDescriptorPool->AllocateDescriptorSet(mMaterialParamDescSetLayout.get(), newDescriptorSetCount);
                mMaterialResourceDescSets = mMaterialDescriptorPool->AllocateDescriptorSet(mMaterialResourceDescSetLayout.get(), newDescriptorSetCount);
                assert(mMaterialDescSets.size() == newDescriptorSetCount && "Failed to AllocateDescriptorSet");
                assert(mMaterialResourceDescSets.size() == newDescriptorSetCount && "Failed to AllocateDescriptorSet");

                // 创建新的材质缓冲区
                uint32_t diffCount = newDescriptorSetCount - mLastDescriptorSetCount;
                for (int i = 0; i < diffCount; i++) {
                        mMaterialBuffers.push_back(std::make_shared<AdVKBuffer>(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(UnlitMaterialUbo), nullptr, true));
                }
                LOG_W("{0}: {1} -> {2} E.", __FUNCTION__, mLastDescriptorSetCount, newDescriptorSetCount);
                mLastDescriptorSetCount = newDescriptorSetCount;
        }

        /**
         * @brief 更新帧UBO描述符集，传递投影矩阵、视图矩阵等每帧数据。
         *
         * @param renderTarget 渲染目标对象，用于获取帧缓冲信息。
         */
        void AdUnlitMaterialSystem::UpdateFrameUboDescSet(AdRenderTarget* renderTarget) {
                AdApplication* app = GetApp();
                AdVKDevice* device = GetDevice();

                AdVKFrameBuffer* frameBuffer = renderTarget->GetFrameBuffer();
                glm::ivec2 resolution = { frameBuffer->GetWidth(), frameBuffer->GetHeight() };

                // 构造帧UBO数据
                FrameUbo frameUbo = {
                    .projMat = GetProjMat(renderTarget),
                    .viewMat = GetViewMat(renderTarget),
                    .resolution = resolution,
                    .frameId = static_cast<uint32_t>(app->GetFrameIndex()),
                    .time = app->GetStartTimeSecond()
                };

                // 写入缓冲区并更新描述符集
                mFrameUboBuffer->WriteData(&frameUbo);
                VkDescriptorBufferInfo bufferInfo = DescriptorSetWriter::BuildBufferInfo(mFrameUboBuffer->GetHandle(), 0, sizeof(frameUbo));
                VkWriteDescriptorSet bufferWrite = DescriptorSetWriter::WriteBuffer(mFrameUboDescSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
                DescriptorSetWriter::UpdateDescriptorSets(device->GetHandle(), { bufferWrite });
        }

        /**
         * @brief 更新材质参数描述符集，传递材质参数（如颜色、纹理参数等）。
         *
         * @param descSet 材质参数描述符集句柄。
         * @param material 无光照材质对象，包含参数数据。
         */
        void AdUnlitMaterialSystem::UpdateMaterialParamsDescSet(VkDescriptorSet descSet, AdUnlitMaterial* material) {
                AdVKDevice* device = GetDevice();

                AdVKBuffer* materialBuffer = mMaterialBuffers[material->GetIndex()].get();

                // 获取材质参数并更新纹理参数
                UnlitMaterialUbo params = material->GetParams();

                const TextureView* texture0 = material->GetTextureView(UNLIT_MAT_BASE_COLOR_0);
                if (texture0) {
                        AdMaterial::UpdateTextureParams(texture0, &params.textureParam0);
                }

                const TextureView* texture1 = material->GetTextureView(UNLIT_MAT_BASE_COLOR_1);
                if (texture1) {
                        AdMaterial::UpdateTextureParams(texture1, &params.textureParam1);
                }

                // 写入缓冲区并更新描述符集
                materialBuffer->WriteData(&params);
                VkDescriptorBufferInfo bufferInfo = DescriptorSetWriter::BuildBufferInfo(materialBuffer->GetHandle(), 0, sizeof(params));
                VkWriteDescriptorSet bufferWrite = DescriptorSetWriter::WriteBuffer(descSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
                DescriptorSetWriter::UpdateDescriptorSets(device->GetHandle(), { bufferWrite });
        }

        /**
         * @brief 更新材质资源描述符集，传递纹理采样器和图像视图。
         *
         * @param descSet 材质资源描述符集句柄。
         * @param material 无光照材质对象，包含纹理资源。
         */
        void AdUnlitMaterialSystem::UpdateMaterialResourceDescSet(VkDescriptorSet descSet, AdUnlitMaterial* material) {
                AdVKDevice* device = GetDevice();

                // 获取纹理资源
                const TextureView* texture0 = material->GetTextureView(UNLIT_MAT_BASE_COLOR_0);
                const TextureView* texture1 = material->GetTextureView(UNLIT_MAT_BASE_COLOR_1);

                // 确保有有效的纹理资源
                TextureView defaultView;
                defaultView.texture = mDefaultTexture.get();
                defaultView.sampler = mDefaultSampler.get();

                // 如果没有设置纹理，使用默认纹理
                if (!texture0) texture0 = &defaultView;
                if (!texture1) texture1 = &defaultView;

                // 确保纹理和采样器都有效
                if (!texture0->texture || !texture0->sampler) {
                        texture0 = &defaultView;
                }
                if (!texture1->texture || !texture1->sampler) {
                        texture1 = &defaultView;
                }

                // 构建图像信息
                VkDescriptorImageInfo textureInfo0 = DescriptorSetWriter::BuildImageInfo(
                        texture0->sampler->GetHandle(),
                        texture0->texture->GetImageView()->GetHandle()
                );

                VkDescriptorImageInfo textureInfo1 = DescriptorSetWriter::BuildImageInfo(
                        texture1->sampler->GetHandle(),
                        texture1->texture->GetImageView()->GetHandle()
                );

                VkWriteDescriptorSet textureWrite0 = DescriptorSetWriter::WriteImage(
                        descSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &textureInfo0
                );
                VkWriteDescriptorSet textureWrite1 = DescriptorSetWriter::WriteImage(
                        descSet, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &textureInfo1
                );

                DescriptorSetWriter::UpdateDescriptorSets(device->GetHandle(), { textureWrite0, textureWrite1 });
        }
}