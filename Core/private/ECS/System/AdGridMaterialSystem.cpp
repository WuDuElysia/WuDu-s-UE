#include "ECS/System/AdGridMaterialSystem.h"

#include "AdFileUtil.h"
#include "AdApplication.h"
#include "Graphic/AdVKPipeline.h"
#include "Graphic/AdVKDescriptorSet.h"
#include "Graphic/AdVKBuffer.h"
#include "Graphic/AdVKFrameBuffer.h"
#include "Render/AdRenderTarget.h"
#include "Render/AdMaterial.h"

namespace WuDu {

    void AdGridMaterialSystem::OnInit(AdVKRenderPass* renderPass) {
        AdVKDevice* device = GetDevice();

        // 创建 Frame UBO 描述符集布局（1 个 UBO binding，vertex + fragment 阶段可见）
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

        // 创建管线布局（仅 Frame UBO 描述符集，无推送常量）
        ShaderLayout shaderLayout = {
            .descriptorSetLayouts = {
                mFrameUboDescSetLayout->GetHandle()
            },
            .pushConstants = {}
        };
        mPipelineLayout = std::make_shared<AdVKPipelineLayout>(
            device,
            AD_RES_SHADER_DIR"Scene_Grid.vert",
            AD_RES_SHADER_DIR"Scene_Grid.frag",
            shaderLayout
        );

        // 创建图形管线
        mPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mPipelineLayout.get());
        mPipeline->SetVertexInputState({}, {});       // 空顶点输入（无顶点缓冲）
        mPipeline->EnableDepthTest();                  // 启用深度测试和深度写入
        mPipeline->SetColorBlendAttachmentState(     // 启用 alpha 混合（SrcAlpha / OneMinusSrcAlpha）
            VK_TRUE,
            VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
            VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD
        );
        mPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
        mPipeline->SetSubPassIndex(4);                // 最终 subpass（索引 4）
        mPipeline->Create();

        // 创建描述符池（1 个 UBO 描述符）和描述符集
        std::vector<VkDescriptorPoolSize> poolSizes = {
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1
            }
        };
        mDescriptorPool = std::make_shared<AdVKDescriptorPool>(device, 1, poolSizes);
        mFrameUboDescSet = mDescriptorPool->AllocateDescriptorSet(mFrameUboDescSetLayout.get(), 1)[0];

        // 创建 Frame UBO 缓冲
        mFrameUboBuffer = std::make_shared<AdVKBuffer>(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(FrameUbo), nullptr, true);
    }

    void AdGridMaterialSystem::OnRender(VkCommandBuffer cmdBuffer, AdRenderTarget* renderTarget) {
        // 更新帧 UBO 数据
        UpdateFrameUboDescSet(renderTarget);

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

        // 绑定描述符集
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mPipelineLayout->GetHandle(), 0, 1, &mFrameUboDescSet, 0, nullptr);

        // 绘制全屏三角形（3 个顶点）
        vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
    }

    void AdGridMaterialSystem::OnDestroy() {
        mPipeline.reset();
        mPipelineLayout.reset();
    }

    void AdGridMaterialSystem::UpdateFrameUboDescSet(AdRenderTarget* renderTarget) {
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

}
