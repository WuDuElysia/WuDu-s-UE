# WuDu Engine 模型渲染实现指南

## 1. 概述

本文档详细介绍如何在WuDu Engine中实现完整模型的渲染流程。我们将从模型加载完成后的数据结构开始，逐步讲解如何将模型数据传输到GPU，如何配置渲染管线，以及如何执行实际的绘制操作。

## 2. 模型渲染流程总览

完整的模型渲染流程包括以下关键步骤：

```
模型加载完成 → 顶点/索引数据处理 → GPU缓冲区创建 → 材质资源加载 → 渲染管线配置 → 绘制命令生成 → 执行渲染
```

## 3. 数据结构与内存管理

### 3.1 模型数据结构回顾

从资源系统加载的模型数据结构如下：

```cpp
// 模型顶点结构（已在资源系统文档中定义）
struct ModelVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

// 模型网格结构
struct ModelMesh {
    std::string Name;
    std::shared_ptr<AdVKBuffer> VertexBuffer;
    std::shared_ptr<AdVKBuffer> IndexBuffer;
    uint32_t VertexCount;
    uint32_t IndexCount;
    uint32_t MaterialIndex;
};

// 模型材质结构
struct ModelMaterial {
    std::string Name;
    glm::vec3 DiffuseColor;
    glm::vec3 SpecularColor;
    float Shininess;
    std::string DiffuseTexturePath;
    std::string NormalTexturePath;
    std::string SpecularTexturePath;
};

// 模型资源类
class AdModelResource : public AdResource {
public:
    // ...
    const std::vector<ModelMesh>& GetMeshes() const { return mMeshes; }
    const std::vector<ModelMaterial>& GetMaterials() const { return mMaterials; }
    // ...
};
```

### 3.2 GPU缓冲区创建

模型加载完成后，需要将顶点和索引数据传输到GPU内存中：

```cpp
// Core/private/Resource/AdModelResource.cpp (部分实现)
#include "Resource/AdModelResource.h"
#include "Render/AdVKBuffer.h"
#include "Render/AdVKDevice.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace ade {

// 处理单个网格的顶点和索引数据
void AdModelResource::ProcessMesh(aiMesh* mesh, const aiScene* scene, AdVKDevice* device) {
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> indices;
    
    // 填充顶点数据
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        ModelVertex vertex;
        
        // 位置
        vertex.Position.x = mesh->mVertices[i].x;
        vertex.Position.y = mesh->mVertices[i].y;
        vertex.Position.z = mesh->mVertices[i].z;
        
        // 法线
        if (mesh->HasNormals()) {
            vertex.Normal.x = mesh->mNormals[i].x;
            vertex.Normal.y = mesh->mNormals[i].y;
            vertex.Normal.z = mesh->mNormals[i].z;
        }
        
        // 纹理坐标
        if (mesh->mTextureCoords[0]) {
            vertex.TexCoord.x = mesh->mTextureCoords[0][i].x;
            vertex.TexCoord.y = mesh->mTextureCoords[0][i].y;
        }
        
        // 切线和副切线
        if (mesh->HasTangentsAndBitangents()) {
            vertex.Tangent.x = mesh->mTangents[i].x;
            vertex.Tangent.y = mesh->mTangents[i].y;
            vertex.Tangent.z = mesh->mTangents[i].z;
            
            vertex.Bitangent.x = mesh->mBitangents[i].x;
            vertex.Bitangent.y = mesh->mBitangents[i].y;
            vertex.Bitangent.z = mesh->mBitangents[i].z;
        }
        
        vertices.push_back(vertex);
    }
    
    // 填充索引数据
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }
    
    // 创建GPU顶点缓冲区
    VkDeviceSize bufferSize = sizeof(ModelVertex) * vertices.size();
    
    // 1. 创建staging buffer（CPU可访问）
    std::shared_ptr<AdVKBuffer> stagingBuffer = std::make_shared<AdVKBuffer>(
        device,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    
    // 2. 将顶点数据复制到staging buffer
    void* data;
    vkMapMemory(device->GetLogicalDevice(), stagingBuffer->GetMemory(), 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(device->GetLogicalDevice(), stagingBuffer->GetMemory());
    
    // 3. 创建顶点缓冲区（GPU本地）
    std::shared_ptr<AdVKBuffer> vertexBuffer = std::make_shared<AdVKBuffer>(
        device,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    
    // 4. 将数据从staging buffer复制到顶点缓冲区
    device->CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);
    
    // 5. 创建索引缓冲区（类似顶点缓冲区的流程）
    bufferSize = sizeof(uint32_t) * indices.size();
    std::shared_ptr<AdVKBuffer> indexStagingBuffer = std::make_shared<AdVKBuffer>(
        device,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    
    data = nullptr;
    vkMapMemory(device->GetLogicalDevice(), indexStagingBuffer->GetMemory(), 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device->GetLogicalDevice(), indexStagingBuffer->GetMemory());
    
    std::shared_ptr<AdVKBuffer> indexBuffer = std::make_shared<AdVKBuffer>(
        device,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    
    device->CopyBuffer(indexStagingBuffer, indexBuffer, bufferSize);
    
    // 6. 创建ModelMesh对象并添加到模型中
    ModelMesh modelMesh;
    modelMesh.Name = mesh->mName.C_Str();
    modelMesh.VertexBuffer = vertexBuffer;
    modelMesh.IndexBuffer = indexBuffer;
    modelMesh.VertexCount = static_cast<uint32_t>(vertices.size());
    modelMesh.IndexCount = static_cast<uint32_t>(indices.size());
    modelMesh.MaterialIndex = mesh->mMaterialIndex;
    
    mMeshes.push_back(modelMesh);
}

} // namespace ade
```

## 4. 渲染管线配置

### 4.1 顶点输入布局

为了让GPU正确解析顶点数据，需要定义顶点输入布局：

```cpp
// 顶点绑定描述
std::vector<VkVertexInputBindingDescription> GetVertexBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindings;
    
    // 一个绑定，每个顶点数据占sizeof(ModelVertex)字节
    bindings.push_back({
        0,                                      // 绑定索引
        sizeof(ModelVertex),                    // 顶点步长
        VK_VERTEX_INPUT_RATE_VERTEX             // 每个顶点更新一次
    });
    
    return bindings;
}

// 顶点属性描述
std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributes;
    
    // 位置属性 (location = 0)
    attributes.push_back({
        0,                                      // 属性位置
        0,                                      // 绑定索引
        VK_FORMAT_R32G32B32_SFLOAT,             // 数据格式
        offsetof(ModelVertex, Position)         // 在顶点结构中的偏移量
    });
    
    // 法线属性 (location = 1)
    attributes.push_back({
        1,
        0,
        VK_FORMAT_R32G32B32_SFLOAT,
        offsetof(ModelVertex, Normal)
    });
    
    // 纹理坐标属性 (location = 2)
    attributes.push_back({
        2,
        0,
        VK_FORMAT_R32G32_SFLOAT,
        offsetof(ModelVertex, TexCoord)
    });
    
    // 切线属性 (location = 3)
    attributes.push_back({
        3,
        0,
        VK_FORMAT_R32G32B32_SFLOAT,
        offsetof(ModelVertex, Tangent)
    });
    
    // 副切线属性 (location = 4)
    attributes.push_back({
        4,
        0,
        VK_FORMAT_R32G32B32_SFLOAT,
        offsetof(ModelVertex, Bitangent)
    });
    
    return attributes;
}
```

### 4.2 着色器配置

模型渲染需要适配的顶点和片段着色器。以下是简化版本：

#### 顶点着色器 (model.vert)

```glsl
#version 450

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
    mat4 viewProjectionMatrix;
} pushConsts;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec2 vTexCoord;
layout(location = 2) out vec3 vWorldPosition;

void main() {
    vec4 worldPos = pushConsts.modelMatrix * vec4(aPosition, 1.0);
    vWorldPosition = worldPos.xyz;
    vNormal = mat3(transpose(inverse(pushConsts.modelMatrix))) * aNormal;
    vTexCoord = aTexCoord;
    gl_Position = pushConsts.viewProjectionMatrix * worldPos;
}
```

#### 片段着色器 (model.frag)

```glsl
#version 450

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec3 vWorldPosition;

layout(set = 0, binding = 0) uniform sampler2D uDiffuseTexture;

layout(push_constant) uniform PushConstants {
    vec3 diffuseColor;
} materialConsts;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    
    // 简单的漫反射光照计算
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);
    
    // 环境光
    vec3 ambient = vec3(0.3);
    
    // 采样纹理
    vec4 texColor = texture(uDiffuseTexture, vTexCoord);
    
    // 最终颜色
    vec3 result = (ambient + diffuse) * materialConsts.diffuseColor * texColor.rgb;
    fragColor = vec4(result, 1.0);
}
```

### 4.3 渲染管线创建

创建专门用于模型渲染的管线：

```cpp
// 创建模型渲染管线
VkPipeline CreateModelPipeline(VkDevice device, VkPipelineLayout pipelineLayout, 
                              VkRenderPass renderPass, VkExtent2D extent) {
    // 顶点输入阶段
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    auto vertexBindings = GetVertexBindingDescriptions();
    auto vertexAttrs = GetVertexAttributeDescriptions();
    
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexBindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttrs.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttrs.data();
    
    // 输入装配阶段
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // 视口和裁剪
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    // 光栅化阶段
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // 深度和模板测试
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    // 颜色混合
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    // 动态状态
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };
    
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    
    // 着色器阶段
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
        CreateShaderStage(device, "path/to/model.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
        CreateShaderStage(device, "path/to/model.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
    };
    
    // 管线创建信息
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    
    // 创建管线
    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create model rendering pipeline!");
    }
    
    // 清理着色器模块
    for (auto& stage : shaderStages) {
        vkDestroyShaderModule(device, stage.module, nullptr);
    }
    
    return pipeline;
}
```

## 5. ECS集成与组件设计

### 5.1 模型组件

创建用于ECS的模型组件：

```cpp
// Core/public/ECS/Components/AdModelComponent.h
#ifndef AD_MODEL_COMPONENT_H
#define AD_MODEL_COMPONENT_H

#include <string>
#include <memory>
#include "Resource/AdModelResource.h"

namespace ade {

class AdModelComponent {
public:
    AdModelComponent() = default;
    explicit AdModelComponent(const std::string& modelPath);
    
    void SetModelPath(const std::string& path);
    const std::string& GetModelPath() const { return mModelPath; }
    
    std::shared_ptr<AdModelResource> GetModelResource() const { return mModelResource; }
    
private:
    std::string mModelPath;
    std::shared_ptr<AdModelResource> mModelResource;
};

} // namespace ade

#endif // AD_MODEL_COMPONENT_H
```

```cpp
// Core/private/ECS/Components/AdModelComponent.cpp
#include "ECS/Components/AdModelComponent.h"
#include "Resource/AdResourceManager.h"

namespace ade {

AdModelComponent::AdModelComponent(const std::string& modelPath) {
    SetModelPath(modelPath);
}

void AdModelComponent::SetModelPath(const std::string& path) {
    mModelPath = path;
    mModelResource = AdResourceManager::GetInstance()->Load<AdModelResource>(path);
}

} // namespace ade
```

### 5.2 模型渲染系统

创建系统来处理模型渲染：

```cpp
// Core/private/Render/AdModelRenderSystem.cpp
#include "Render/AdModelRenderSystem.h"
#include "ECS/Components/AdTransformComponent.h"
#include "ECS/Components/AdModelComponent.h"
#include "Render/AdVKDevice.h"
#include "Render/AdVKCommandBuffer.h"
#include "Adlog.h"

namespace ade {

AdModelRenderSystem::AdModelRenderSystem(AdVKDevice* device, VkRenderPass renderPass, VkExtent2D extent) 
    : mDevice(device), mExtent(extent) {
    // 创建渲染管线
    CreatePipelineLayout();
    CreatePipeline(renderPass);
}

AdModelRenderSystem::~AdModelRenderSystem() {
    vkDestroyPipelineLayout(mDevice->GetLogicalDevice(), mPipelineLayout, nullptr);
    vkDestroyPipeline(mDevice->GetLogicalDevice(), mPipeline, nullptr);
}

void AdModelRenderSystem::Render(AdVKCommandBuffer* commandBuffer, entt::registry& registry, 
                                 const glm::mat4& viewProjectionMatrix) {
    // 绑定管线
    vkCmdBindPipeline(commandBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
    
    // 获取所有具有模型和变换组件的实体
    auto view = registry.view<AdTransformComponent, AdModelComponent>();
    
    for (auto entity : view) {
        auto& transform = view.get<AdTransformComponent>(entity);
        auto& modelComp = view.get<AdModelComponent>(entity);
        
        auto modelResource = modelComp.GetModelResource();
        if (!modelResource || !modelResource->IsLoaded()) {
            continue; // 模型未加载完成，跳过渲染
        }
        
        // 计算模型矩阵
        glm::mat4 modelMatrix = transform.GetWorldMatrix();
        
        // 设置推送常量（模型矩阵和视图投影矩阵）
        struct PushConstants {
            glm::mat4 modelMatrix;
            glm::mat4 viewProjectionMatrix;
        } pushConsts;
        
        pushConsts.modelMatrix = modelMatrix;
        pushConsts.viewProjectionMatrix = viewProjectionMatrix;
        
        vkCmdPushConstants(
            commandBuffer->GetHandle(),
            mPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(PushConstants),
            &pushConsts
        );
        
        // 获取模型的网格和材质
        const auto& meshes = modelResource->GetMeshes();
        const auto& materials = modelResource->GetMaterials();
        
        // 渲染每个网格
        for (const auto& mesh : meshes) {
            // 绑定顶点缓冲区
            VkBuffer vertexBuffers[] = { mesh.VertexBuffer->GetHandle() };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer->GetHandle(), 0, 1, vertexBuffers, offsets);
            
            // 绑定索引缓冲区
            vkCmdBindIndexBuffer(
                commandBuffer->GetHandle(),
                mesh.IndexBuffer->GetHandle(),
                0,
                VK_INDEX_TYPE_UINT32
            );
            
            // 如果材质索引有效，绑定材质
            if (mesh.MaterialIndex < materials.size()) {
                const auto& material = materials[mesh.MaterialIndex];
                
                // 绑定材质的纹理描述符集
                // 注意：这里需要假设材质已经加载了纹理并创建了描述符集
                // 实际实现中需要完善材质加载和描述符集管理
                if (material.DescriptorSet != VK_NULL_HANDLE) {
                    vkCmdBindDescriptorSets(
                        commandBuffer->GetHandle(),
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        mPipelineLayout,
                        0,  // 第一个描述符集
                        1,
                        &material.DescriptorSet,
                        0,
                        nullptr
                    );
                }
                
                // 设置材质推送常量
                struct MaterialPushConstants {
                    glm::vec3 diffuseColor;
                } materialConsts;
                
                materialConsts.diffuseColor = material.DiffuseColor;
                
                vkCmdPushConstants(
                    commandBuffer->GetHandle(),
                    mPipelineLayout,
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    sizeof(PushConstants),  // 偏移量，跳过顶点阶段的常量
                    sizeof(MaterialPushConstants),
                    &materialConsts
                );
            }
            
            // 执行绘制命令
            vkCmdDrawIndexed(
                commandBuffer->GetHandle(),
                mesh.IndexCount,
                1,  // 实例数
                0,  // 索引偏移
                0,  // 顶点偏移
                0   // 实例偏移
            );
        }
    }
}

void AdModelRenderSystem::CreatePipelineLayout() {
    // 推送常量布局
    VkPushConstantRange pushConstantRanges[2] = {};
    
    // 顶点阶段的推送常量
    pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRanges[0].offset = 0;
    pushConstantRanges[0].size = sizeof(glm::mat4) * 2;  // model + viewProjection
    
    // 片段阶段的推送常量
    pushConstantRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRanges[1].offset = sizeof(glm::mat4) * 2;
    pushConstantRanges[1].size = sizeof(glm::vec3);  // diffuse color
    
    // 描述符集布局（用于纹理等资源）
    // 注意：实际实现中需要完善描述符集布局的创建
    VkDescriptorSetLayoutBinding textureBinding = {};
    textureBinding.binding = 0;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = 1;
    textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &textureBinding;
    
    VkDescriptorSetLayout descriptorSetLayout;
    vkCreateDescriptorSetLayout(mDevice->GetLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayout);
    
    // 管线布局创建
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 2;
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    
    if (vkCreatePipelineLayout(mDevice->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
    
    // 清理描述符集布局（它已经被管线布局引用）
    vkDestroyDescriptorSetLayout(mDevice->GetLogicalDevice(), descriptorSetLayout, nullptr);
}

void AdModelRenderSystem::CreatePipeline(VkRenderPass renderPass) {
    // 这里应该调用之前定义的CreateModelPipeline函数
    // 实际实现中需要根据引擎的着色器加载机制调整
    mPipeline = CreateModelPipeline(mDevice->GetLogicalDevice(), mPipelineLayout, renderPass, mExtent);
}

} // namespace ade
```

## 6. 完整渲染流程示例

### 6.1 主应用集成

在主应用中集成模型渲染：

```cpp
// Core/private/AdApplication.cpp (部分代码)
#include "Render/AdModelRenderSystem.h"
#include "ECS/Components/AdTransformComponent.h"
#include "ECS/Components/AdModelComponent.h"

void AdApplication::Initialize() {
    // 初始化其他系统...
    
    // 创建模型渲染系统
    mModelRenderSystem = std::make_unique<AdModelRenderSystem>(
        mRenderContext->GetDevice(),
        mRenderContext->GetRenderPass(),
        mWindow->GetExtent()
    );
    
    // 加载并创建一个模型实体
    auto entity = mRegistry.create();
    
    // 添加变换组件
    mRegistry.emplace<AdTransformComponent>(entity, 
        glm::vec3(0.0f, 0.0f, 0.0f),  // 位置
        glm::vec3(0.0f),              // 旋转
        glm::vec3(1.0f)               // 缩放
    );
    
    // 添加模型组件
    mRegistry.emplace<AdModelComponent>(entity, "resources/models/suzanne.obj");
}

void AdApplication::Render() {
    // 开始帧
    mRenderContext->BeginFrame();
    
    // 获取命令缓冲区
    auto commandBuffer = mRenderContext->GetCurrentCommandBuffer();
    
    // 开始渲染通道
    mRenderContext->BeginRenderPass(commandBuffer);
    
    // 渲染模型
    glm::mat4 viewMatrix = mCamera.GetViewMatrix();
    glm::mat4 projectionMatrix = mCamera.GetProjectionMatrix();
    glm::mat4 viewProjection = projectionMatrix * viewMatrix;
    
    mModelRenderSystem->Render(commandBuffer, mRegistry, viewProjection);
    
    // 渲染其他内容...
    
    // 结束渲染通道
    mRenderContext->EndRenderPass(commandBuffer);
    
    // 结束帧
    mRenderContext->EndFrame();
}
```

## 7. 性能优化考虑

### 7.1 批处理绘制

减少绘制调用次数：

```cpp
// 按材质分组绘制相同材质的网格
void AdModelRenderSystem::Render(AdVKCommandBuffer* commandBuffer, entt::registry& registry, 
                                 const glm::mat4& viewProjectionMatrix) {
    // 1. 收集所有需要渲染的模型网格并按材质分组
    std::unordered_map<VkDescriptorSet, std::vector<std::pair<glm::mat4, const ModelMesh*>>> materialMeshMap;
    
    auto view = registry.view<AdTransformComponent, AdModelComponent>();
    for (auto entity : view) {
        auto& transform = view.get<AdTransformComponent>(entity);
        auto& modelComp = view.get<AdModelComponent>(entity);
        
        auto modelResource = modelComp.GetModelResource();
        if (!modelResource || !modelResource->IsLoaded()) continue;
        
        glm::mat4 modelMatrix = transform.GetWorldMatrix();
        const auto& meshes = modelResource->GetMeshes();
        const auto& materials = modelResource->GetMaterials();
        
        for (const auto& mesh : meshes) {
            if (mesh.MaterialIndex < materials.size()) {
                const auto& material = materials[mesh.MaterialIndex];
                materialMeshMap[material.DescriptorSet].emplace_back(modelMatrix, &mesh);
            }
        }
    }
    
    // 2. 按材质分组绘制
    vkCmdBindPipeline(commandBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
    
    for (const auto& [descriptorSet, meshPairs] : materialMeshMap) {
        // 绑定材质描述符集
        if (descriptorSet != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(
                commandBuffer->GetHandle(),
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                mPipelineLayout,
                0, 1, &descriptorSet, 0, nullptr
            );
        }
        
        // 绘制该材质的所有网格
        for (const auto& [modelMatrix, mesh] : meshPairs) {
            // 设置模型矩阵推送常量
            struct PushConstants {
                glm::mat4 modelMatrix;
                glm::mat4 viewProjectionMatrix;
            } pushConsts;
            
            pushConsts.modelMatrix = modelMatrix;
            pushConsts.viewProjectionMatrix = viewProjectionMatrix;
            
            vkCmdPushConstants(
                commandBuffer->GetHandle(),
                mPipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(PushConstants),
                &pushConsts
            );
            
            // 绑定缓冲区并绘制
            VkBuffer vertexBuffers[] = { mesh->VertexBuffer->GetHandle() };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer->GetHandle(), 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer->GetHandle(), mesh->IndexBuffer->GetHandle(), 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer->GetHandle(), mesh->IndexCount, 1, 0, 0, 0);
        }
    }
}
```

### 7.2 实例化渲染

对于多个相同模型的实例，使用实例化渲染：

```cpp
// 实例化数据结构
struct InstanceData {
    glm::mat4 modelMatrix;
    glm::vec3 color;
};

// 创建实例缓冲区
VkBuffer CreateInstanceBuffer(AdVKDevice* device, const std::vector<InstanceData>& instances) {
    VkDeviceSize bufferSize = sizeof(InstanceData) * instances.size();
    
    // 创建staging buffer
    auto stagingBuffer = std::make_shared<AdVKBuffer>(
        device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    
    // 复制数据
    void* data;
    vkMapMemory(device->GetLogicalDevice(), stagingBuffer->GetMemory(), 0, bufferSize, 0, &data);
    memcpy(data, instances.data(), (size_t)bufferSize);
    vkUnmapMemory(device->GetLogicalDevice(), stagingBuffer->GetMemory());
    
    // 创建实例缓冲区
    auto instanceBuffer = std::make_shared<AdVKBuffer>(
        device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    
    // 复制到GPU
    device->CopyBuffer(stagingBuffer, instanceBuffer, bufferSize);
    
    return instanceBuffer;
}

// 实例化绘制
vkCmdDrawIndexed(
    commandBuffer,
    mesh.IndexCount,
    instanceCount,  // 实例数量
    0, 0, 0
);
```

## 8. 调试与常见问题

### 8.1 调试技巧

1. **启用Vulkan验证层**：在开发阶段启用完整的验证层，捕获内存泄漏和错误。

2. **检查顶点数据**：
   ```cpp
   // 调试顶点数据
   LOG_D("Mesh {0}: {1} vertices, {2} indices", mesh.Name, mesh.VertexCount, mesh.IndexCount);
   if (mesh.VertexCount > 0) {
       // 映射顶点缓冲区查看数据
       float* vertexData;
       vkMapMemory(device, vertexBuffer->GetMemory(), 0, sizeof(ModelVertex), 0, (void**)&vertexData);
       LOG_D("First vertex: ({0}, {1}, {2})", vertexData[0], vertexData[1], vertexData[2]);
       vkUnmapMemory(device, vertexBuffer->GetMemory());
   }