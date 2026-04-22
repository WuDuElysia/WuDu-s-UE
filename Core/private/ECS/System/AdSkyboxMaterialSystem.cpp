#include "ECS/System/AdSkyboxMaterialSystem.h"

#include "AdFileUtil.h"
#include "AdApplication.h"
#include "Graphic/AdVKPipeline.h"
#include "Graphic/AdVKRenderPass.h"
#include "Graphic/AdVKDescriptorSet.h"
#include "Graphic/AdVKBuffer.h"
#include "Graphic/AdVKFrameBuffer.h"
#include "Render/AdRenderTarget.h"
#include "Render/AdMaterial.h"

namespace WuDu {

    void AdSkyboxMaterialSystem::OnInit(AdVKRenderPass* renderPass) {
        AdVKDevice* device = GetDevice();

        // 创建 FrameUbo 描述符集布局（set=0, binding=0, UBO, VERTEX | FRAGMENT）
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

        // 创建 SkyboxUbo 描述符集布局（set=1, binding=0, UBO, FRAGMENT）
        {
            const std::vector<VkDescriptorSetLayoutBinding> bindings = {
                {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                }
            };
            mSkyboxUboDescSetLayout = std::make_shared<AdVKDescriptorSetLayout>(device, bindings);
        }

        // 创建管线布局（两个描述符集布局，无推送常量）
        ShaderLayout shaderLayout = {
            .descriptorSetLayouts = {
                mFrameUboDescSetLayout->GetHandle(),
                mSkyboxUboDescSetLayout->GetHandle()
            },
            .pushConstants = {}
        };
        mPipelineLayout = std::make_shared<AdVKPipelineLayout>(
            device,
            AD_RES_SHADER_DIR"Skybox_Aurora.vert",
            AD_RES_SHADER_DIR"Skybox_Aurora.frag",
            shaderLayout
        );

        // 从 renderPass 查询天空盒应使用的 subpass
        // 前向渲染（1个subpass）：使用 subpass 0
        // 延迟渲染（多个subpass）：使用最后一个有深度附件的 subpass（通常是后处理阶段）
        const auto& subPasses = renderPass->GetSubPasses();
        uint32_t skyboxSubpass = 0;
        VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
        uint32_t colorAttachmentCount = 1;

        if (subPasses.size() > 1) {
            // 延迟渲染：找最后一个有深度附件的 subpass
            for (int i = static_cast<int>(subPasses.size()) - 1; i >= 0; i--) {
                if (!subPasses[i].depthStencilAttachments.empty()) {
                    skyboxSubpass = static_cast<uint32_t>(i);
                    break;
                }
            }
        }

        if (!subPasses.empty() && skyboxSubpass < subPasses.size()) {
            sampleCount = subPasses[skyboxSubpass].sampleCount;
            colorAttachmentCount = static_cast<uint32_t>(subPasses[skyboxSubpass].colorAttachments.size());
        }

        // 创建图形管线
        mPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mPipelineLayout.get());
        mPipeline->SetVertexInputState({}, {});       // 空顶点输入（无顶点缓冲）
        mPipeline->SetDepthStencilState({             // 深度测试 LESS_OR_EQUAL，禁用深度写入
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        });
        mPipeline->SetRasterizationState({            // 禁用面剔除
            .cullMode = VK_CULL_MODE_NONE,
        });
        if (sampleCount > VK_SAMPLE_COUNT_1_BIT) {
            mPipeline->SetMultisampleState(sampleCount, VK_FALSE);
        }
        mPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
        mPipeline->SetSubPassIndex(skyboxSubpass);
        mPipeline->SetColorAttachmentCount(colorAttachmentCount);
        mPipeline->Create();

        // 创建描述符池（2 个 UBO 描述符，maxSets=2）
        std::vector<VkDescriptorPoolSize> poolSizes = {
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 2
            }
        };
        mDescriptorPool = std::make_shared<AdVKDescriptorPool>(device, 2, poolSizes);

        // 分配 FrameUbo 和 SkyboxUbo 描述符集
        mFrameUboDescSet = mDescriptorPool->AllocateDescriptorSet(mFrameUboDescSetLayout.get(), 1)[0];
        mSkyboxUboDescSet = mDescriptorPool->AllocateDescriptorSet(mSkyboxUboDescSetLayout.get(), 1)[0];

        // 创建 FrameUbo 和 SkyboxUbo 缓冲
        mFrameUboBuffer = std::make_shared<AdVKBuffer>(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(FrameUbo), nullptr, true);
        mSkyboxUboBuffer = std::make_shared<AdVKBuffer>(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(SkyboxUbo), nullptr, true);
    }

    void AdSkyboxMaterialSystem::OnRender(VkCommandBuffer cmdBuffer, AdRenderTarget* renderTarget) {
        // 更新 UBO 数据
        UpdateFrameUboDescSet(renderTarget);
        UpdateSkyboxUboDescSet();

        // 绑定管线
        mPipeline->Bind(cmdBuffer);

        // 设置动态视口和裁剪区域
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

        // 绑定两个描述符集（set=0: FrameUbo, set=1: SkyboxUbo）
        VkDescriptorSet descriptorSets[] = {
            mFrameUboDescSet,
            mSkyboxUboDescSet
        };
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mPipelineLayout->GetHandle(), 0, 2, descriptorSets, 0, nullptr);

        // 绘制全屏三角形（3 个顶点）
        vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
    }

    void AdSkyboxMaterialSystem::OnDestroy() {
        mPipeline.reset();
        mPipelineLayout.reset();
    }

    void AdSkyboxMaterialSystem::UpdateFrameUboDescSet(AdRenderTarget* renderTarget) {
        AdApplication* app = GetApp();
        AdVKDevice* device = GetDevice();

        AdVKFrameBuffer* frameBuffer = renderTarget->GetFrameBuffer();
        glm::ivec2 resolution = { frameBuffer->GetWidth(), frameBuffer->GetHeight() };

        // 构造 FrameUbo 数据
        FrameUbo frameUbo = {
            .projMat = GetProjMat(renderTarget),
            .viewMat = GetViewMat(renderTarget),
            .camPos = GetCameraPosition(renderTarget),
            ._pad0 = 0.0f,
            .resolution = resolution,
            .frameId = static_cast<uint32_t>(app->GetFrameIndex()),
            .time = app->GetStartTimeSecond()
        };

        // 写入缓冲并更新描述符集
        mFrameUboBuffer->WriteData(&frameUbo);
        VkDescriptorBufferInfo bufferInfo = DescriptorSetWriter::BuildBufferInfo(mFrameUboBuffer->GetHandle(), 0, sizeof(FrameUbo));
        VkWriteDescriptorSet bufferWrite = DescriptorSetWriter::WriteBuffer(mFrameUboDescSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
        DescriptorSetWriter::UpdateDescriptorSets(device->GetHandle(), { bufferWrite });
    }

    void AdSkyboxMaterialSystem::UpdateSkyboxUboDescSet() {
        AdVKDevice* device = GetDevice();

        // 写入 SkyboxUbo 数据到缓冲并更新描述符集
        mSkyboxUboBuffer->WriteData(&mSkyboxParams);
        VkDescriptorBufferInfo bufferInfo = DescriptorSetWriter::BuildBufferInfo(mSkyboxUboBuffer->GetHandle(), 0, sizeof(SkyboxUbo));
        VkWriteDescriptorSet bufferWrite = DescriptorSetWriter::WriteBuffer(mSkyboxUboDescSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo);
        DescriptorSetWriter::UpdateDescriptorSets(device->GetHandle(), { bufferWrite });
    }

}
