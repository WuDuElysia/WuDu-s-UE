# PBR 延迟渲染管线规划

## 1. 概述

本文档详细规划了一个基于您现有 Vulkan 基础设施的 PBR 延迟渲染管线，充分利用了 subpass 支持。该设计在您当前的前向渲染系统基础上构建，同时引入了延迟渲染在复杂光照场景下的性能优势。

## 2. 现有架构分析

### 2.1 可复用的关键组件

- **AdPBRMaterial**：材质数据结构（无需修改）
- **光源组件**：AdDirectionalLightComponent、AdPointLightComponent、AdSpotLightComponent（无需修改）
- **ECS 系统**：沿用与 AdPBRForwardMaterialSystem 相同的 ECS 模式
- **Vulkan 基础设施**：AdVKPipeline、AdVKRenderPass、AdVKDescriptorSet 等
- **着色器布局**：类似的描述符集布局模式

### 2.2 前向渲染与延迟渲染对比

| 方面 | 前向渲染 | 延迟渲染 |
|------|----------|----------|
| **光照成本** | O(N*L)，其中 N=物体数量，L=光源数量 | O(S + L)，其中 S=屏幕像素，L=光源数量 |
| **材质多样性** | 受限于着色器复杂度 | 无限制（所有材质使用相同的光照着色器） |
| **透明支持** | 原生支持 | 需要混合方法 |
| **内存使用** | 较低（无 GBuffer） | 较高（GBuffer 附件） |
| **过绘制处理** | 较差（所有物体都被渲染） | 较好（光照前进行深度测试） |

## 3. 延迟管线架构

### 3.1 基于 Subpass 的管线设计

延迟管线将在单个渲染通道中使用 **5 个 subpass**，利用 Vulkan 的 subpass 依赖关系实现高效的内存访问和同步：

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                 渲染通道                                         │
│  ┌─────────────────┐  ┌─────────────────┐  ┌───────────────┐  ┌───────────────┐  │
│  │  Subpass 0:     │  │  Subpass 1:     │  │  Subpass 2:   │  │  Subpass 3:   │  │
│  │  几何阶段       │─►│  直接光照阶段   │─►│  IBL 阶段     │─►│  合并阶段     │─►│
│  └─────────────────┘  └─────────────────┘  └───────────────┘  └───────────────┘  │
│                                                                  │                │
│                                                                  ▼                │
│                                                          ┌───────────────┐        │
│                                                          │  Subpass 4:   │        │
│                                                          │  后处理阶段   │        │
│                                                          └───────────────┘        │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Subpass 依赖关系

| 源 Subpass | 目标 Subpass | 源阶段掩码 | 目标阶段掩码 | 源访问掩码 | 目标访问掩码 |
|------------|--------------|------------|--------------|------------|--------------|
| 0          | 1            | COLOR_ATTACHMENT_OUTPUT_BIT | FRAGMENT_SHADER_BIT | COLOR_ATTACHMENT_WRITE_BIT | INPUT_ATTACHMENT_READ_BIT |
| 1          | 2            | COLOR_ATTACHMENT_OUTPUT_BIT | FRAGMENT_SHADER_BIT | COLOR_ATTACHMENT_WRITE_BIT | INPUT_ATTACHMENT_READ_BIT |
| 2          | 3            | COLOR_ATTACHMENT_OUTPUT_BIT | FRAGMENT_SHADER_BIT | COLOR_ATTACHMENT_WRITE_BIT | INPUT_ATTACHMENT_READ_BIT |
| 3          | 4            | COLOR_ATTACHMENT_OUTPUT_BIT | FRAGMENT_SHADER_BIT | COLOR_ATTACHMENT_WRITE_BIT | INPUT_ATTACHMENT_READ_BIT |

## 4. GBuffer 设计

### 4.1 GBuffer 附件

| 附件 | 格式 | 用途 | 描述 |
|------|------|------|------|
| GBuffer 0 | `VK_FORMAT_R8G8B8A8_SRGB` | 颜色附件 | 基础颜色 (RGB) + Alpha (A) |
| GBuffer 1 | `VK_FORMAT_R16G16B16A16_SFLOAT` | 颜色附件 | 法线 (RGB) + 自发光强度 (A) |
| GBuffer 2 | `VK_FORMAT_R8G8B8A8_UNORM` | 颜色附件 | 金属度 (R) + 粗糙度 (G) + AO (B) + 填充 (A) |
| 深度缓冲 | `VK_FORMAT_D32_SFLOAT` | 深度附件 | 用于深度测试的深度信息 |

### 4.2 GBuffer 内存布局

```
┌─────────────────────────────────────────────────┐
│                GBuffer 附件 0                   │
│  ┌─────────────────────────────────────────────┐│
│  │ 基础颜色 RGB (8位) │ Alpha (8位)            ││
│  └─────────────────────────────────────────────┘│
├─────────────────────────────────────────────────┤
│                GBuffer 附件 1                   │
│  ┌─────────────────────────────────────────────┐│
│  │ 法线 RGB (16位浮点) │ 自发光 (16位)          ││
│  └─────────────────────────────────────────────┘│
├─────────────────────────────────────────────────┤
│                GBuffer 附件 2                   │
│  ┌─────────────────────────────────────────────┐│
│  │ 金属度 (8位) │ 粗糙度 (8位) │ AO (8位)        ││
│  └─────────────────────────────────────────────┘│
├─────────────────────────────────────────────────┤
│                深度缓冲                         │
│  ┌─────────────────────────────────────────────┐│
│  │ 深度 (32位浮点)                              ││
│  └─────────────────────────────────────────────┘│
└─────────────────────────────────────────────────┘
```

## 5. 渲染通道配置

### 5.1 附件描述

```cpp
std::vector<AdVKRenderPass::Attachment> attachments = {
    // GBuffer 0: 基础颜色 + Alpha
    {
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
    },
    // GBuffer 1: 法线 + 自发光
    {
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
    },
    // GBuffer 2: 金属度 + 粗糙度 + AO
    {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
    },
    // 深度缓冲
    {
        .format = VK_FORMAT_D32_SFLOAT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
    },
    // 直接光照累积缓冲
    {
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
    },
    // IBL 累积缓冲
    {
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
    },
    // 合并后的光照缓冲
    {
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
    },
    // 后处理输出缓冲
    {
        .format = device->GetSettings().surfaceFormat,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    }
};
```

### 5.2 Subpass 配置

```cpp
std::vector<AdVKRenderPass::RenderSubPass> subPasses = {
    // Subpass 0: 几何阶段
    {
        .inputAttachments = {},
        .colorAttachments = {0, 1, 2},  // GBuffer 0, 1, 2
        .depthStencilAttachments = {3},  // 深度缓冲
        .sampleCount = VK_SAMPLE_COUNT_1_BIT
    },
    // Subpass 1: 直接光照阶段
    {
        .inputAttachments = {0, 1, 2, 3},  // GBuffer 0-2 + 深度
        .colorAttachments = {4},  // 直接光照累积
        .depthStencilAttachments = {},
        .sampleCount = VK_SAMPLE_COUNT_1_BIT
    },
    // Subpass 2: IBL 阶段
    {
        .inputAttachments = {0, 1, 2, 3, 4},  // GBuffer 0-2 + 深度 + 直接光照
        .colorAttachments = {5},  // IBL 累积
        .depthStencilAttachments = {},
        .sampleCount = VK_SAMPLE_COUNT_1_BIT
    },
    // Subpass 3: 合并阶段
    {
        .inputAttachments = {0, 1, 4, 5},  // 基础颜色 + 自发光 + 直接光照 + IBL
        .colorAttachments = {6},  // 合并后的光照
        .depthStencilAttachments = {},
        .sampleCount = VK_SAMPLE_COUNT_1_BIT
    },
    // Subpass 4: 后处理阶段
    {
        .inputAttachments = {6},  // 合并后的光照
        .colorAttachments = {7},  // 最终输出
        .depthStencilAttachments = {},
        .sampleCount = VK_SAMPLE_COUNT_1_BIT
    }
};
```

## 6. 管线设计

### 6.1 延迟材质系统架构

创建一个新的系统 `AdPBRDeferredMaterialSystem`，遵循与 `AdPBRForwardMaterialSystem` 相同的模式：

```cpp
class AdPBRDeferredMaterialSystem : public AdMaterialSystem {
public:
    void OnInit(AdVKRenderPass* renderPass) override;
    void OnRender(VkCommandBuffer cmdBuffer, AdRenderTarget* renderTarget) override;
    void OnDestroy() override;
    
private:
    // 管线布局
    std::shared_ptr<AdVKDescriptorSetLayout> mFrameUboDescSetLayout;
    std::shared_ptr<AdVKDescriptorSetLayout> mMaterialParamDescSetLayout;
    std::shared_ptr<AdVKDescriptorSetLayout> mLightUboDescSetLayout;
    std::shared_ptr<AdVKDescriptorSetLayout> mMaterialResourceDescSetLayout;
    std::shared_ptr<AdVKDescriptorSetLayout> mGbufferDescSetLayout;
    std::shared_ptr<AdVKDescriptorSetLayout> mLightingDescSetLayout;
    std::shared_ptr<AdVKDescriptorSetLayout> mPostProcessDescSetLayout;
    
    // 各 subpass 的管线
    std::shared_ptr<AdVKPipelineLayout> mGbufferPipelineLayout;
    std::shared_ptr<AdVKPipeline> mGbufferPipeline;
    
    std::shared_ptr<AdVKPipelineLayout> mDirectLightingPipelineLayout;
    std::shared_ptr<AdVKPipeline> mDirectLightingPipeline;
    
    std::shared_ptr<AdVKPipelineLayout> mIBLPipelineLayout;
    std::shared_ptr<AdVKPipeline> mIBLPipeline;
    
    std::shared_ptr<AdVKPipelineLayout> mMergePipelineLayout;
    std::shared_ptr<AdVKPipeline> mMergePipeline;
    
    std::shared_ptr<AdVKPipelineLayout> mPostProcessPipelineLayout;
    std::shared_ptr<AdVKPipeline> mPostProcessPipeline;
    
    // 描述符集
    VkDescriptorSet mFrameUboDescSet;
    VkDescriptorSet mLightUboDescSet;
    VkDescriptorSet mPostProcessDescSet;
    std::vector<VkDescriptorSet> mMaterialDescSets;
    std::vector<VkDescriptorSet> mMaterialResourceDescSets;
    
    // 缓冲区
    std::shared_ptr<AdVKBuffer> mFrameUboBuffer;
    std::shared_ptr<AdVKBuffer> mLightUboBuffer;
    std::shared_ptr<AdVKBuffer> mPostProcessBuffer;
    std::vector<std::shared_ptr<AdVKBuffer>> mMaterialBuffers;
    
    // 其他成员...
};
```

### 6.2 管线设置

#### 6.2.1 几何阶段管线 (Subpass 0)

```cpp
// 在 OnInit() 中
{
    // GBuffer 管线布局（与前向类似，但使用不同的着色器）
    ShaderLayout shaderLayout = {
        .descriptorSetLayouts = {
            mFrameUboDescSetLayout->GetHandle(),
            mMaterialParamDescSetLayout->GetHandle(),
            mMaterialResourceDescSetLayout->GetHandle()
        },
        .pushConstants = { modelPC }
    };
    mGbufferPipelineLayout = std::make_shared<AdVKPipelineLayout>(
        device,
        AD_RES_SHADER_DIR"PBR_Deferred_GBuffer.vert",
        AD_RES_SHADER_DIR"PBR_Deferred_GBuffer.frag",
        shaderLayout
    );
    
    mGbufferPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mGbufferPipelineLayout.get());
    mGbufferPipeline->SetVertexInputState(vertexBindings, vertexAttrs);
    mGbufferPipeline->EnableDepthTest();
    mGbufferPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
    mGbufferPipeline->SetSubPassIndex(0);  // 绑定到 Subpass 0
    mGbufferPipeline->Create();
}
```

#### 6.2.2 直接光照管线 (Subpass 1)

```cpp
// 在 OnInit() 中
{
    // 直接光照管线布局
    ShaderLayout shaderLayout = {
        .descriptorSetLayouts = {
            mFrameUboDescSetLayout->GetHandle(),
            mLightUboDescSetLayout->GetHandle(),
            mGbufferDescSetLayout->GetHandle()
        }
    };
    mDirectLightingPipelineLayout = std::make_shared<AdVKPipelineLayout>(
        device,
        AD_RES_SHADER_DIR"Fullscreen.vert",
        AD_RES_SHADER_DIR"PBR_Deferred_DirectLighting.frag",
        shaderLayout
    );
    
    mDirectLightingPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mDirectLightingPipelineLayout.get());
    // 全屏四边形顶点输入（仅位置）
    mDirectLightingPipeline->SetVertexInputState(/* 全屏顶点绑定 */);
    mDirectLightingPipeline->DisableDepthTest();
    mDirectLightingPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
    mDirectLightingPipeline->SetSubPassIndex(1);  // 绑定到 Subpass 1
    mDirectLightingPipeline->Create();
}
```

#### 6.2.3 IBL 管线 (Subpass 2)

```cpp
// 在 OnInit() 中
{
    // IBL 管线布局
    ShaderLayout shaderLayout = {
        .descriptorSetLayouts = {
            mFrameUboDescSetLayout->GetHandle(),
            mGbufferDescSetLayout->GetHandle(),
            mIBLResourceDescSetLayout->GetHandle()
        }
    };
    mIBLPipelineLayout = std::make_shared<AdVKPipelineLayout>(
        device,
        AD_RES_SHADER_DIR"Fullscreen.vert",
        AD_RES_SHADER_DIR"PBR_Deferred_IBL.frag",
        shaderLayout
    );
    
    mIBLPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mIBLPipelineLayout.get());
    mIBLPipeline->SetVertexInputState(/* 全屏顶点绑定 */);
    mIBLPipeline->DisableDepthTest();
    mIBLPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
    mIBLPipeline->SetSubPassIndex(2);  // 绑定到 Subpass 2
    mIBLPipeline->Create();
}
```

#### 6.2.4 合并管线 (Subpass 3)

```cpp
// 在 OnInit() 中
{
    // 合并管线布局
    ShaderLayout shaderLayout = {
        .descriptorSetLayouts = {
            mGbufferDescSetLayout->GetHandle(),
            mLightingDescSetLayout->GetHandle()
        }
    };
    mMergePipelineLayout = std::make_shared<AdVKPipelineLayout>(
        device,
        AD_RES_SHADER_DIR"Fullscreen.vert",
        AD_RES_SHADER_DIR"PBR_Deferred_Merge.frag",
        shaderLayout
    );
    
    mMergePipeline = std::make_shared<AdVKPipeline>(device, renderPass, mMergePipelineLayout.get());
    mMergePipeline->SetVertexInputState(/* 全屏顶点绑定 */);
    mMergePipeline->DisableDepthTest();
    mMergePipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
    mMergePipeline->SetSubPassIndex(3);  // 绑定到 Subpass 3
    mMergePipeline->Create();
}
```

#### 6.2.5 后处理管线 (Subpass 4)

```cpp
// 在 OnInit() 中
{
    // 后处理管线布局
    ShaderLayout shaderLayout = {
        .descriptorSetLayouts = {
            mFrameUboDescSetLayout->GetHandle(),
            mPostProcessDescSetLayout->GetHandle()
        }
    };
    mPostProcessPipelineLayout = std::make_shared<AdVKPipelineLayout>(
        device,
        AD_RES_SHADER_DIR"Fullscreen.vert",
        AD_RES_SHADER_DIR"PBR_Deferred_PostProcess.frag",
        shaderLayout
    );
    
    mPostProcessPipeline = std::make_shared<AdVKPipeline>(device, renderPass, mPostProcessPipelineLayout.get());
    mPostProcessPipeline->SetVertexInputState(/* 全屏顶点绑定 */);
    mPostProcessPipeline->DisableDepthTest();
    mPostProcessPipeline->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
    mPostProcessPipeline->SetSubPassIndex(4);  // 绑定到 Subpass 4
    mPostProcessPipeline->Create();
}
```

### 6.3 渲染循环

```cpp
void AdPBRDeferredMaterialSystem::OnRender(VkCommandBuffer cmdBuffer, AdRenderTarget* renderTarget) {
    // 更新帧 UBO
    UpdateFrameUboDescSet(renderTarget);
    
    // 更新光源 UBO
    UpdateLightUboDescSet();
    
    // 更新后处理 UBO
    UpdatePostProcessDescSet();
    
    // 开始渲染通道（自动处理 subpass 依赖关系）
    renderPass->Begin(cmdBuffer, frameBuffer, clearValues);
    
    // Subpass 0: 几何阶段
    {
        mGbufferPipeline->Bind(cmdBuffer);
        
        // 更新动态视口和裁剪区域
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
        
        // 绑定帧 UBO 描述符集
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mGbufferPipelineLayout->GetHandle(), 0, 1, &mFrameUboDescSet, 0, nullptr);
        
        // 渲染所有网格（与前向渲染类似）
        auto view = registry.view<AdTransformComponent, AdPBRMaterialComponent>();
        view.each([this, &cmdBuffer](AdTransformComponent& transComp, AdPBRMaterialComponent& materialComp) {
            // 如有需要，更新材质描述符集
            // 绑定材质描述符集
            // 推送模型矩阵
            // 绘制网格
        });
    }
    
    // Subpass 1: 直接光照阶段
    {
        mDirectLightingPipeline->Bind(cmdBuffer);
        
        // 绑定描述符集（GBuffer、光源等）
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mDirectLightingPipelineLayout->GetHandle(), 0, 3, descriptorSets, 0, nullptr);
        
        // 绘制全屏四边形
        vkCmdDraw(cmdBuffer, 3, 1, 0, 0); // 覆盖屏幕的三角形
    }
    
    // Subpass 2: IBL 阶段
    {
        mIBLPipeline->Bind(cmdBuffer);
        
        // 绑定描述符集（GBuffer、IBL 资源等）
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mIBLPipelineLayout->GetHandle(), 0, 3, descriptorSets, 0, nullptr);
        
        // 绘制全屏四边形
        vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
    }
    
    // Subpass 3: 合并阶段
    {
        mMergePipeline->Bind(cmdBuffer);
        
        // 绑定描述符集（光照结果、GBuffer 等）
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mMergePipelineLayout->GetHandle(), 0, 2, descriptorSets, 0, nullptr);
        
        // 绘制全屏四边形
        vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
    }
    
    // Subpass 4: 后处理阶段
    {
        mPostProcessPipeline->Bind(cmdBuffer);
        
        // 绑定描述符集（合并后的光照、后处理参数等）
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mPostProcessPipelineLayout->GetHandle(), 0, 2, descriptorSets, 0, nullptr);
        
        // 绘制全屏四边形
        vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
    }
    
    // 结束渲染通道
    renderPass->End(cmdBuffer);
}
```

## 7. 着色器设计

### 7.1 几何阶段着色器

**顶点着色器 (`PBR_Deferred_GBuffer.vert`)**:
```glsl
#version 450

layout(location=0) in vec3 a_Position;
layout(location=1) in vec2 a_TexCoord;
layout(location=2) in vec3 a_Normal;
layout(location=3) in vec3 a_Tangent;
layout(location=4) in vec3 a_Bitangent;

layout(push_constant) uniform ModelPC {
    mat4 modelMat;
} u_ModelPC;

layout(set=0, binding=0) uniform FrameUbo {
    mat4 projMat;
    mat4 viewMat;
    vec2 resolution;
    uint frameId;
    float time;
} u_FrameUbo;

layout(location=0) out vec3 v_WorldPos;
out vec2 v_Texcoord;
out mat3 v_TBN;
out vec3 v_Normal;

void main() {
    // 将位置变换到世界空间
    vec4 worldPos = u_ModelPC.modelMat * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    
    // 将法线变换到世界空间
    mat3 normalMat = transpose(inverse(mat3(u_ModelPC.modelMat)));
    v_Normal = normalMat * a_Normal;
    
    // 构建用于法线映射的 TBN 矩阵
    vec3 T = normalize(normalMat * a_Tangent);
    vec3 B = normalize(normalMat * a_Bitangent);
    vec3 N = normalize(v_Normal);
    v_TBN = mat3(T, B, N);
    
    // 传递纹理坐标
    v_Texcoord = a_TexCoord;
    
    // 变换到裁剪空间
    gl_Position = u_FrameUbo.projMat * u_FrameUbo.viewMat * worldPos;
}
```

**片段着色器 (`PBR_Deferred_GBuffer.frag`)**:
```glsl
#version 450

layout(location=0) in vec3 v_WorldPos;
in vec2 v_Texcoord;
in mat3 v_TBN;
in vec3 v_Normal;

// GBuffer 输出
layout(location=0) out vec4 outBaseColor;
layout(location=1) out vec4 outNormalEmissive;
layout(location=2) out vec4 outMetallicRoughnessAO;

// 材质参数
layout(set=1, binding=0) uniform PBRMaterialParams {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float aoFactor;
    float emissiveFactor;
} u_Material;

// 材质纹理
layout(set=2, binding=0) uniform sampler2D u_BaseColorTexture;
layout(set=2, binding=1) uniform sampler2D u_NormalTexture;
layout(set=2, binding=2) uniform sampler2D u_MetallicRoughnessTexture;
layout(set=2, binding=3) uniform sampler2D u_AoTexture;
layout(set=2, binding=4) uniform sampler2D u_EmissiveTexture;

void main() {
    // 采样基础颜色
    vec4 baseColor = texture(u_BaseColorTexture, v_Texcoord) * u_Material.baseColorFactor;
    outBaseColor = baseColor;
    
    // 采样法线贴图并转换到世界空间
    vec3 normalMap = texture(u_NormalTexture, v_Texcoord).rgb * 2.0 - 1.0;
    vec3 worldNormal = normalize(v_TBN * normalMap);
    
    // 采样金属度-粗糙度贴图
    vec2 metallicRoughness = texture(u_MetallicRoughnessTexture, v_Texcoord).gb;
    float metallic = metallicRoughness.x * u_Material.metallicFactor;
    float roughness = metallicRoughness.y * u_Material.roughnessFactor;
    
    // 采样 AO 贴图
    float ao = texture(u_AoTexture, v_Texcoord).r * u_Material.aoFactor;
    
    // 采样自发光贴图
    vec3 emissive = texture(u_EmissiveTexture, v_Texcoord).rgb * u_Material.emissiveFactor;
    
    // 写入 GBuffer
    outNormalEmissive = vec4(worldNormal, emissive.r); // 将自发光存储在 alpha 通道
    outMetallicRoughnessAO = vec4(metallic, roughness, ao, 0.0);
}
```

### 7.2 直接光照着色器

**片段着色器 (`PBR_Deferred_DirectLighting.frag`)**:
```glsl
#version 450

layout(location=0) in vec2 v_ScreenPos;

layout(location=0) out vec4 outDirectLighting;

// GBuffer 输入
layout(set=2, binding=0) uniform sampler2D u_GbufferBaseColor;
layout(set=2, binding=1) uniform sampler2D u_GbufferNormalEmissive;
layout(set=2, binding=2) uniform sampler2D u_GbufferMetallicRoughnessAO;
layout(set=2, binding=3) uniform sampler2D u_GbufferDepth;

// 光源 UBO
layout(set=1, binding=0) uniform LightingUbo {
    vec3 ambientColor;
    float ambientIntensity;
    uint numLights;
    Light lights[MAX_LIGHTS];
} u_LightingUbo;

// 帧 UBO
layout(set=0, binding=0) uniform FrameUbo {
    mat4 projMat;
    mat4 viewMat;
    mat4 invProjMat;
    mat4 invViewMat;
    vec2 resolution;
    vec3 cameraPos;
} u_FrameUbo;

void main() {
    // 从深度缓冲重建世界位置
    vec3 worldPos = ReconstructWorldPos(v_ScreenPos, u_GbufferDepth, u_FrameUbo.invProjMat, u_FrameUbo.invViewMat);
    
    // 采样 GBuffer
    vec4 baseColor = texture(u_GbufferBaseColor, v_ScreenPos);
    vec4 normalEmissive = texture(u_GbufferNormalEmissive, v_ScreenPos);
    vec4 metallicRoughnessAO = texture(u_GbufferMetallicRoughnessAO, v_ScreenPos);
    
    vec3 normal = normalize(normalEmissive.rgb);
    float metallic = metallicRoughnessAO.r;
    float roughness = metallicRoughnessAO.g;
    float ao = metallicRoughnessAO.b;
    
    // 计算所有光源的光照
    vec3 lighting = vec3(0.0);
    
    for (uint i = 0; i < u_LightingUbo.numLights; i++) {
        Light light = u_LightingUbo.lights[i];
        vec3 lightDir;
        float attenuation = 1.0;
        
        // 根据光源类型计算光方向和衰减
        if (light.type == LIGHT_TYPE_DIRECTIONAL) {
            lightDir = normalize(-light.direction);
        } else if (light.type == LIGHT_TYPE_POINT) {
            vec3 lightToPos = worldPos - light.position;
            float distance = length(lightToPos);
            lightDir = normalize(lightToPos);
            attenuation = 1.0 / (1.0 + light.attenuation.linear * distance + light.attenuation.quadratic * distance * distance);
        } else if (light.type == LIGHT_TYPE_SPOT) {
            // 聚光灯计算...
        }
        
        // PBR 光照计算
        vec3 V = normalize(u_FrameUbo.cameraPos - worldPos);
        vec3 H = normalize(-lightDir + V);
        
        // 计算 F、G、D 项（Cook-Torrance BRDF）
        // ... PBR 光照实现 ...
        
        vec3 radiance = light.color * light.intensity * attenuation;
        vec3 finalLight = radiance * brdf * dot(normal, -lightDir);
        
        lighting += finalLight;
    }
    
    outDirectLighting = vec4(lighting, 1.0);
}
```

### 7.3 IBL 着色器

**片段着色器 (`PBR_Deferred_IBL.frag`)**:
- 采样 GBuffer 和直接光照结果
- 使用 HDR 环境贴图计算环境光照
- 使用预计算的辐照度贴图和 BRDF LUT
- 输出 IBL 贡献

### 7.4 合并着色器

**片段着色器 (`PBR_Deferred_Merge.frag`)**:
- 合并直接光照和 IBL 结果
- 添加自发光贡献
- 应用初步色调映射
- 输出合并后的光照

### 7.5 后处理着色器

**片段着色器 (`PBR_Deferred_PostProcess.frag`)**:
```glsl
#version 450

layout(location=0) in vec2 v_ScreenPos;

layout(location=0) out vec4 outFinalColor;

// 输入纹理
layout(set=1, binding=0) uniform sampler2D u_MergedLighting;

// 后处理参数
layout(set=0, binding=0) uniform PostProcessUbo {
    float exposure;
    bool enableFXAA;
    float fxaaQuality;
    bool enableBloom;
    float bloomThreshold;
    float bloomIntensity;
} u_PostProcessUbo;

// 帧 UBO
layout(set=0, binding=1) uniform FrameUbo {
    vec2 resolution;
    float time;
} u_FrameUbo;

// FXAA 函数声明
vec3 FXAA(vec2 uv, sampler2D tex, vec2 resolution);

// Bloom 函数声明
vec3 Bloom(vec2 uv, sampler2D tex, float threshold, float intensity);

// 色调映射函数
vec3 ToneMap(vec3 color, float exposure) {
    // ACES 色调映射
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    
    color *= exposure;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main() {
    vec2 uv = v_ScreenPos / u_FrameUbo.resolution;
    
    // 采样合并后的光照
    vec3 color = texture(u_MergedLighting, uv).rgb;
    
    // 应用 Bloom（如果启用）
    if (u_PostProcessUbo.enableBloom) {
        color += Bloom(uv, u_MergedLighting, u_PostProcessUbo.bloomThreshold, u_PostProcessUbo.bloomIntensity);
    }
    
    // 应用 FXAA（如果启用）
    if (u_PostProcessUbo.enableFXAA) {
        color = FXAA(uv, u_MergedLighting, u_FrameUbo.resolution);
    }
    
    // 应用色调映射
    color = ToneMap(color, u_PostProcessUbo.exposure);
    
    // 应用伽马校正
    color = pow(color, vec3(1.0/2.2));
    
    outFinalColor = vec4(color, 1.0);
}
```

## 8. 实现步骤

### 8.1 第一阶段：核心基础设施

1. **创建 GBuffer 渲染通道**：实现包含 5 个 subpass 的渲染通道
2. **创建 AdPBRDeferredMaterialSystem**：设置系统结构，包含每个 subpass 对应的管线
3. **实现几何阶段**：创建着色器和管线，用于填充 GBuffer
4. **测试 GBuffer 输出**：验证 GBuffer 附件是否正确填充

### 8.2 第二阶段：光照实现

5. **实现直接光照阶段**：创建着色器和管线，用于计算直接光照
6. **实现 IBL 阶段**：添加 IBL 支持，使用环境贴图
7. **实现合并阶段**：组合光照结果，添加初步色调映射
8. **测试光照输出**：验证光照计算是否正确

### 8.3 第三阶段：后处理实现

9. **实现后处理阶段**：添加色调映射、抗锯齿（FXAA/TAA）和 Bloom 效果
10. **测试后处理效果**：验证后处理效果是否符合预期

### 8.4 第四阶段：优化和集成

11. **优化光源剔除**：为大量光源实现 tiled/clustered 光照
12. **集成透明物体处理**：添加混合前向渲染，用于处理透明物体
13. **添加阴影支持**：为延迟渲染实现阴影映射
14. **性能分析**：识别并优化瓶颈

## 9. 性能考虑

- **Subpass 效率**：Vulkan subpass 允许高效的附件访问，无需显式同步
- **GBuffer 带宽**：在保持所需精度的同时，最小化 GBuffer 大小
- **光源剔除**：为包含数百/数千个光源的场景实现 tiled/clustered 光照
- **早期 Z 剔除**：确保在几何阶段启用深度测试
- **Mipmap 使用**：为纹理采样使用适当的 mip 级别

## 10. 调试建议

- **GBuffer 可视化**：添加调试视图，检查各个 GBuffer 附件
- **光照阶段调试**：添加选项，单独显示直接光照、IBL 和最终结果
- **后处理调试**：为每个后处理效果添加开关，便于单独调试
- **性能标记**：添加 Vulkan 性能标记，用于分析
- **验证层**：在开发过程中启用 Vulkan 验证层

## 11. 结论

本延迟渲染管线设计充分利用了您现有的 Vulkan 基础设施，同时引入了延迟渲染在复杂光照场景下的性能优势。通过使用 subpass，我们确保了渲染阶段之间高效的内存访问和同步。

该实现遵循模块化方法，允许您增量构建和测试每个组件，同时尽可能重用现有代码库。这个设计为未来增强（如高级光照效果、阴影映射和进一步性能优化）提供了坚实的基础。