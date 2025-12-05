# WuDu Engine 资源系统设计文档

## 1. 资源系统概述

资源系统是游戏引擎的核心组件之一，负责管理和加载各种游戏资源，如模型、纹理、着色器、音频等。本文档详细介绍如何在WuDu Engine中设计和实现一个完整的资源系统，重点关注外部模型的导入和渲染流程，以及相关的技术实现原理。

## 2. 当前项目架构分析

### 2.1 现有渲染架构

根据对现有代码的分析，WuDu Engine已经具备了以下渲染相关组件：

- **AdApplication**: 应用程序主类，负责游戏主循环和生命周期管理
- **AdRenderContext**: 渲染上下文，封装了图形设备和交换链
- **AdVKDevice/AdVKSwapchain/AdVKImage**: Vulkan API的封装
- **AdRenderer**: 处理渲染循环和同步
- **AdGeometryUtil**: 用于创建基本几何体（如立方体）

### 2.2 现有资源管理

目前项目中资源管理相对简单：
- 着色器文件直接从`Resource/Shader/`目录加载
- 纹理资源位于`Resource/Texture/`目录
- 缺乏统一的资源管理系统和模型加载功能

## 3. 资源系统设计

### 3.1 整体架构

资源系统将采用以下架构：

```
AdResourceManager (资源管理器) - 单例模式
    ├── AdResourceLoader (资源加载器)
    │   ├── AdModelLoader (模型加载器)
    │   ├── AdTextureLoader (纹理加载器)
    │   └── AdShaderLoader (着色器加载器)
    ├── AdResourceCache (资源缓存)
    └── AdResourceDatabase (资源数据库)

AdResource (资源基类)
    ├── AdModelResource (模型资源)
    ├── AdTextureResource (纹理资源)
    └── AdShaderResource (着色器资源)
```

### 3.2 核心类设计

#### 3.2.1 资源基类 (AdResource)

```cpp
// Core/public/Resource/AdResource.h
#ifndef AD_RESOURCE_H
#define AD_RESOURCE_H

#include <string>
#include <memory>
#include <atomic>
#include <string_view>
#include <random>
#include <sstream>

namespace WuDu {

enum class ResourceState {
    Unloaded,
    Loading,
    Loaded,
    Failed
};

// UUID类型定义
typedef std::string UUID;

// UUID生成器实现
inline UUID GenerateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 12; i++) {
        ss << dis(gen);
    }
    return ss.str();
}

enum class ResourceIdType {
    Path,
    UUID
};

class AdResource {
public:
    AdResource(const std::string& resourcePath) 
        : mPath(resourcePath), mUUID(GenerateUUID()), mState(ResourceState::Unloaded), mRefCount(0) {}
    
    virtual ~AdResource() = default;

    virtual bool Load() = 0;
    virtual void Unload() = 0;
    virtual bool Reload();

    const std::string& GetPath() const { return mPath; }
    const UUID& GetUUID() const { return mUUID; }
    ResourceState GetState() const { return mState; }
    bool IsLoaded() const { return mState == ResourceState::Loaded; }
    
    // 设置资源路径（用于资源移动或重命名）
    void SetPath(const std::string& newPath) { mPath = newPath; }

    void AddReference() { mRefCount.fetch_add(1); }
    void RemoveReference() {
        if (mRefCount.fetch_sub(1) == 1) {
            Unload();
        }
    }

protected:
    std::string mPath;
    UUID mUUID; // 唯一标识符
    ResourceState mState;
    std::atomic<int32_t> mRefCount;
};

} // namespace WuDu

#endif // AD_RESOURCE_H
```

#### 3.2.2 资源管理器 (AdResourceManager)

```cpp
// Core/public/Resource/AdResourceManager.h
#ifndef AD_RESOURCE_MANAGER_H
#define AD_RESOURCE_MANAGER_H

#include "AdResource.h"
#include <unordered_map>
#include <mutex>
#include <functional>
#include <optional>

namespace WuDu {

class AdResourceManager {
public:
    static AdResourceManager* GetInstance();

    template<typename T, typename... Args>
    std::shared_ptr<T> Load(const std::string& path, Args&&... args) {
        static_assert(std::is_base_of<AdResource, T>::value, "T must derive from AdResource");
        
        std::lock_guard<std::mutex> lock(mMutex);
        
        // 首先检查路径是否已存在
        auto pathIt = mPathToUUID.find(path);
        if (pathIt != mPathToUUID.end()) {
            const UUID& uuid = pathIt->second;
            auto uuidIt = mUUIDToResource.find(uuid);
            if (uuidIt != mUUIDToResource.end()) {
                auto resource = std::dynamic_pointer_cast<T>(uuidIt->second.lock());
                if (resource) {
                    return resource;
                }
            }
        }
        
        // 创建新资源，资源内部会生成UUID
        auto resource = std::make_shared<T>(path, std::forward<Args>(args)...);
        if (resource->Load()) {
            const UUID& uuid = resource->GetUUID();
            
            // 更新双映射表
            mUUIDToResource[uuid] = resource;
            mPathToUUID[path] = uuid;
        }
        
        return resource;
    }

    // 通过路径获取资源
    template<typename T>
    std::shared_ptr<T> Get(const std::string& path) {
        static_assert(std::is_base_of<AdResource, T>::value, "T must derive from AdResource");
        
        std::lock_guard<std::mutex> lock(mMutex);
        
        auto pathIt = mPathToUUID.find(path);
        if (pathIt != mPathToUUID.end()) {
            const UUID& uuid = pathIt->second;
            return Get<T>(uuid);
        }
        
        return nullptr;
    }

    // 通过UUID获取资源
    template<typename T>
    std::shared_ptr<T> Get(const UUID& uuid) {
        static_assert(std::is_base_of<AdResource, T>::value, "T must derive from AdResource");
        
        std::lock_guard<std::mutex> lock(mMutex);
        
        auto it = mUUIDToResource.find(uuid);
        if (it != mUUIDToResource.end()) {
            return std::dynamic_pointer_cast<T>(it->second.lock());
        }
        
        return nullptr;
    }

    // 通过UUID获取资源路径
    std::optional<std::string> GetPathByUUID(const UUID& uuid) {
        std::lock_guard<std::mutex> lock(mMutex);
        
        for (const auto& [path, resourceUUID] : mPathToUUID) {
            if (resourceUUID == uuid) {
                return path;
            }
        }
        
        return std::nullopt;
    }

    // 更新资源路径映射
    bool UpdateResourcePath(const UUID& uuid, const std::string& newPath) {
        std::lock_guard<std::mutex> lock(mMutex);
        
        // 移除旧路径映射
        auto oldPathIt = mPathToUUID.begin();
        while (oldPathIt != mPathToUUID.end()) {
            if (oldPathIt->second == uuid) {
                oldPathIt = mPathToUUID.erase(oldPathIt);
                break;
            } else {
                ++oldPathIt;
            }
        }
        
        // 添加新路径映射
        mPathToUUID[newPath] = uuid;
        
        // 更新资源内部路径
        auto resourceIt = mUUIDToResource.find(uuid);
        if (resourceIt != mUUIDToResource.end()) {
            auto resource = resourceIt->second.lock();
            if (resource) {
                // 注意：这里需要AdResource类提供SetPath方法
                // resource->SetPath(newPath);
                return true;
            }
        }
        
        return false;
    }

    void UnloadUnused();
    void UnloadAll();

    void SetResourceRootPath(const std::string& rootPath) {
        mRootPath = rootPath;
    }

    std::string GetFullPath(const std::string& relativePath) const {
        return mRootPath + relativePath;
    }

private:
    AdResourceManager();
    ~AdResourceManager();

    std::string mRootPath;
    
    // 双映射表实现双标识系统
    std::unordered_map<UUID, std::weak_ptr<AdResource>> mUUIDToResource;  // UUID到资源的映射
    std::unordered_map<std::string, UUID> mPathToUUID;  // 路径到UUID的映射
    
    std::mutex mMutex;
};

} // namespace WuDu

#endif // AD_RESOURCE_MANAGER_H
```

#### 3.2.3 模型资源 (AdModelResource)

```cpp
// Core/public/Resource/AdModelResource.h
#ifndef AD_MODEL_RESOURCE_H
#define AD_MODEL_RESOURCE_H

#include "AdResource.h"
#include "Render/AdVKBuffer.h"
#include <vector>
#include <glm/glm.hpp>

namespace WuDu {

struct ModelVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct ModelMesh {
    std::string Name;
    std::shared_ptr<AdVKBuffer> VertexBuffer;
    std::shared_ptr<AdVKBuffer> IndexBuffer;
    uint32_t VertexCount;
    uint32_t IndexCount;
    uint32_t MaterialIndex;
};

struct ModelMaterial {
    std::string Name;
    glm::vec3 DiffuseColor;
    glm::vec3 SpecularColor;
    float Shininess;
    std::string DiffuseTexturePath;
    std::string NormalTexturePath;
    std::string SpecularTexturePath;
};

class AdModelResource : public AdResource {
public:
    AdModelResource(const std::string& modelPath);
    ~AdModelResource();

    bool Load() override;
    void Unload() override;

    const std::vector<ModelMesh>& GetMeshes() const { return mMeshes; }
    const std::vector<ModelMaterial>& GetMaterials() const { return mMaterials; }

private:
    std::vector<ModelMesh> mMeshes;
    std::vector<ModelMaterial> mMaterials;
    std::shared_ptr<AdVKDevice> mDevice;
};

} // namespace WuDu

#endif // AD_MODEL_RESOURCE_H
```

### 3.3 技术实现原理详解

#### 3.3.1 顶点缓冲区配置原理

**顶点缓冲区创建流程**：

1. **数据收集**：
   - 从Assimp导入的模型中提取原始顶点数据（位置、法线、纹理坐标等）
   - 数据存储在CPU内存的临时向量中（如`std::vector<ModelVertex>`）

2. **内存分配与绑定**：
   - 创建VkBuffer对象：向Vulkan设备请求一块GPU内存
   - 设置内存属性：指定内存是GPU本地的、CPU可访问的还是两者共享的
   - 顶点缓冲区需要`VK_BUFFER_USAGE_VERTEX_BUFFER_BIT`使用标志

3. **数据传输**：
   - 分配临时的可映射内存（使用`VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`）
   - 将CPU上的顶点数据复制到映射的内存中
   - 通过`vkFlushMappedMemoryRanges`确保数据被刷新到GPU
   - 可选：使用staging buffer进行更高效的数据传输（适用于大量数据）

4. **内存布局优化**：
   - 顶点属性按顺序排列，确保缓存友好访问
   - 使用合适的对齐方式，避免内存碎片
   - 对于频繁访问的数据，考虑使用设备本地内存

**顶点格式定义**：

在渲染管线创建时，需要明确告知GPU顶点数据的布局：

```cpp
// 顶点绑定描述 - 定义顶点数据的整体布局
std::vector<VkVertexInputBindingDescription> vertexBindings = {
    {
        0,                      // 绑定索引
        sizeof(ModelVertex),    // 顶点步长（每个顶点的字节数）
        VK_VERTEX_INPUT_RATE_VERTEX  // 每个顶点更新一次
    }
};

// 顶点属性描述 - 定义每个顶点属性的具体格式和位置
std::vector<VkVertexInputAttributeDescription> vertexAttrs = {
    {
        0,                                          // 属性位置（对应着色器中的location）
        0,                                          // 绑定索引
        VK_FORMAT_R32G32B32_SFLOAT,                 // 数据格式（3个32位浮点数）
        offsetof(ModelVertex, Position)             // 在顶点结构中的偏移量
    },
    {
        1,
        0,
        VK_FORMAT_R32G32B32_SFLOAT,
        offsetof(ModelVertex, Normal)
    },
    {
        2,
        0,
        VK_FORMAT_R32G32_SFLOAT,
        offsetof(ModelVertex, TexCoord)
    },
    // 切线和副切线（如果需要法线贴图）
    {
        3,
        0,
        VK_FORMAT_R32G32B32_SFLOAT,
        offsetof(ModelVertex, Tangent)
    },
    {
        4,
        0,
        VK_FORMAT_R32G32B32_SFLOAT,
        offsetof(ModelVertex, Bitangent)
    }
};
```

**顶点数据访问**：

在渲染过程中，GPU通过以下方式访问顶点缓冲区：

1. **绑定顶点缓冲区**：
   ```cpp
   VkBuffer vertexBuffers[] = { mesh.VertexBuffer->GetHandle() };
   VkDeviceSize offsets[] = { 0 };
   vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
   ```

2. **顶点着色器访问**：
   ```glsl
   // 顶点着色器中通过location布局限定符访问顶点属性
   layout(location = 0) in vec3 aPosition;
   layout(location = 1) in vec3 aNormal;
   layout(location = 2) in vec2 aTexCoord;
   ```

#### 3.3.2 模型顶点绑定与变换机制

**模型矩阵的作用**：

模型矩阵是一个4x4变换矩阵，用于将模型的局部空间顶点转换到世界空间。它包含了以下变换：

1. **缩放变换**：调整模型的大小
2. **旋转变换**：改变模型的朝向
3. **平移变换**：移动模型在世界中的位置

**模型顶点绑定流程**：

1. **组件关联**：
   - 每个实体包含`AdTransformComponent`和`AdModelComponent`
   - `AdTransformComponent`存储实体的位置、旋转和缩放信息
   - `AdModelComponent`持有模型资源引用

2. **变换矩阵计算**：
   ```cpp
   // AdTransformComponent中计算世界矩阵
   glm::mat4 AdTransformComponent::GetWorldMatrix() const {
       glm::mat4 translate = glm::translate(glm::mat4(1.0f), mPosition);
       glm::mat4 rotate = glm::mat4_cast(mRotation);
       glm::mat4 scale = glm::scale(glm::mat4(1.0f), mScale);
       
       return translate * rotate * scale;
   }
   ```

3. **矩阵传递到GPU**：
   - 使用**推送常量**（Push Constants）：适合小数据量，性能高
     ```cpp
     vkCmdPushConstants(
         commandBuffer,
         mPipelineLayout,
         VK_SHADER_STAGE_VERTEX_BIT,
         0,
         sizeof(glm::mat4),
         &transform[0][0]
     );
     ```
   - 或使用**Uniform Buffer**：适合大数据量，需要更多设置

4. **顶点变换**：
   ```glsl
   // 顶点着色器中应用模型矩阵
   layout(push_constant) uniform PushConstants {
       mat4 modelMatrix;
   } pushConsts;
   
   void main() {
       gl_Position = projectionMatrix * viewMatrix * pushConsts.modelMatrix * vec4(aPosition, 1.0);
   }
   ```

**模型顶点分组原理**：

模型通常由多个网格（Mesh）组成，每个网格具有独立的顶点缓冲区：

1. **网格分离依据**：
   - 不同材质：使用不同材质的部分分离到不同网格
   - 几何特性：复杂程度、可见性等因素
   - 渲染需求：需要特殊处理的部分（如透明部分）

2. **批量处理优化**：
   - 相同材质的网格可以合并，减少绘制调用
   - 使用实例化渲染处理多个相同模型
   - 考虑空间划分算法（如八叉树）优化可见性判断

#### 3.3.3 材质系统与绑定机制

**材质数据结构**：

材质包含视觉属性和相关纹理资源：

```cpp
struct ModelMaterial {
    std::string Name;
    
    // 基础颜色属性
    glm::vec3 DiffuseColor;    // 漫反射颜色
    glm::vec3 SpecularColor;   // 镜面反射颜色
    float Shininess;           // 光泽度
    
    // 纹理资源路径
    std::string DiffuseTexturePath;   // 漫反射纹理
    std::string NormalTexturePath;    // 法线贴图
    std::string SpecularTexturePath;  // 高光贴图
};
```

**材质绑定流程**：

1. **纹理资源加载**：
   - 从文件加载纹理数据到CPU内存
   - 创建VkImage和VkImageView对象
   - 设置纹理采样器（VkSampler）

2. **描述符集准备**：
   - 创建VkDescriptorSetLayout定义资源布局
   - 分配VkDescriptorSet实例
   - 更新描述符集，绑定纹理和材质参数

3. **材质应用**：
   - 在渲染每个网格前绑定对应材质
   - 通过描述符集将材质数据传递给着色器
   - 在片段着色器中采样纹理并应用材质属性

```cpp
// 材质绑定示例
void AdModelRenderer::BindMaterial(const ModelMaterial& material, VkCommandBuffer commandBuffer) {
    // 1. 绑定材质的纹理描述符集
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        mPipelineLayout,
        0,  // 第一个描述符集
        1,  // 描述符集数量
        &material.DescriptorSet,  // 描述符集句柄
        0, nullptr
    );
    
    // 2. 可选：通过推送常量传递材质参数
    MaterialPushConstants pushConstants = {
        .diffuseColor = material.DiffuseColor,
        .specularColor = material.SpecularColor,
        .shininess = material.Shininess
    };
    
    vkCmdPushConstants(
        commandBuffer,
        mPipelineLayout,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        sizeof(glm::mat4),  // 偏移量，跳过模型矩阵
        sizeof(MaterialPushConstants),
        &pushConstants
    );
}
```

**着色器中的材质应用**：

```glsl
// 片段着色器中使用材质
layout(set = 0, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 0, binding = 1) uniform sampler2D normalTexture;
layout(set = 0, binding = 2) uniform sampler2D specularTexture;

layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
} pushConsts;

void main() {
    // 采样纹理
    vec4 diffuse = texture(diffuseTexture, fragTexCoord) * vec4(pushConsts.diffuseColor, 1.0);
    vec3 normal = texture(normalTexture, fragTexCoord).rgb * 2.0 - 1.0;
    float specular = texture(specularTexture, fragTexCoord).r * pushConsts.shininess;
    
    // 计算光照
    // ...
    
    // 输出最终颜色
    fragColor = diffuse * (ambient + diffuseLighting) + specularLighting;
}
```

#### 3.3.4 PBR材质系统扩展

**PBR材质原理**：

基于物理的渲染（PBR）使用更真实的光照模型，材质属性主要包括：

1. **基础颜色（Base Color）**：物体表面的固有颜色
2. **金属度（Metallic）**：控制物体的金属特性（0-1范围）
3. **粗糙度（Roughness）**：控制表面的光滑程度（0-1范围）
4. **法线（Normal）**：提供表面细节
5. **环境光遮蔽（AO）**：模拟环境光照的遮挡效果

**PBR材质数据结构**：

```cpp
struct PBRMaterial {
    std::string Name;
    
    // 基础颜色属性
    glm::vec3 BaseColor;
    float Metallic;
    float Roughness;
    float AO;
    
    // 纹理资源路径
    std::string BaseColorTexturePath;
    std::string MetallicRoughnessTexturePath;  // 金属度和粗糙度可存储在一张纹理的不同通道
    std::string NormalTexturePath;
    std::string AOTexturePath;
};
```

**实现PBR材质系统需要修改的组件**：

1. **资源定义**：
   - 扩展`ModelMaterial`结构，添加PBR属性
   - 或创建新的`PBRMaterial`类继承自基础材质

2. **材质加载**：
   - 更新`AdModelLoader::ProcessMaterials`方法，支持PBR材质属性解析
   - 处理不同模型格式的PBR材质定义（如glTF的PBR Metallic-Roughness）

3. **着色器实现**：
   - 创建新的PBR顶点和片段着色器
   - 实现基于物理的BRDF（双向反射分布函数）
   - 支持直接光照和基于图像的光照（IBL）

4. **渲染管线**：
   - 创建专门的PBR渲染管线
   - 更新描述符集布局以支持PBR材质纹理
   - 添加必要的Uniform缓冲区用于存储PBR相关参数

5. **模型渲染器**：
   - 更新`AdModelRenderer`支持PBR材质
   - 添加材质类型检测和对应的渲染路径

**PBR着色器示例**：

```glsl
// PBR片段着色器核心代码
#version 450

layout(set = 0, binding = 0) uniform sampler2D baseColorTexture;
layout(set = 0, binding = 1) uniform sampler2D metallicRoughnessTexture;
layout(set = 0, binding = 2) uniform sampler2D normalTexture;
layout(set = 0, binding = 3) uniform sampler2D aoTexture;
layout(set = 0, binding = 4) uniform samplerCube irradianceMap;
layout(set = 0, binding = 5) uniform samplerCube prefilterMap;
layout(set = 0, binding = 6) uniform sampler2D brdfLUT;

// PBR材质参数
layout(push_constant) uniform PBRMaterialParams {
    vec3 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float aoFactor;
} material;

// 基于物理的BRDF实现
vec3 calculatePBR(vec3 N, vec3 V, vec3 L, vec3 baseColor, float metallic, float roughness, float ao) {
    // 计算反射率
    vec3 F0 = vec3(0.04); // 默认非金属反射率
    F0 = mix(F0, baseColor, metallic);
    
    // 计算几何阴影因子
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    // BRDF公式
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);
    
    // 最终输出
    float NdotL = max(dot(N, L), 0.0);
    vec3 outgoingRadiance = (kD * baseColor / PI + specular) * radiance * NdotL;
    
    return outgoingRadiance;
}

void main() {
    // 采样基础颜色
    vec4 baseColor = texture(baseColorTexture, fragTexCoord) * vec4(material.baseColorFactor, 1.0);
    
    // 采样金属度和粗糙度
    vec4 mrSample = texture(metallicRoughnessTexture, fragTexCoord);
    float metallic = mrSample.b * material.metallicFactor;  // 假设金属度在B通道
    float roughness = mrSample.g * material.roughnessFactor;  // 假设粗糙度在G通道
    
    // 采样法线贴图
    vec3 normal = texture(normalTexture, fragTexCoord).rgb * 2.0 - 1.0;
    normal = normalize(TBN * normal);  // 将法线从切线空间转换到世界空间
    
    // 采样AO贴图
    float ao = texture(aoTexture, fragTexCoord).r * material.aoFactor;
    
    // 计算PBR着色
    vec3 Lo = calculatePBRDirectLight(normal, viewDir, baseColor.rgb, metallic, roughness, ao);
    vec3 IBL = calculatePBRImageBasedLighting(normal, viewDir, baseColor.rgb, metallic, roughness, ao);
    
    vec3 ambient = vec3(0.03) * baseColor.rgb * ao;
    vec3 color = ambient + Lo + IBL;
    
    // 色调映射和gamma校正
    color = color / (color + vec3(1.0));  // Reinhard色调映射
    color = pow(color, vec3(1.0/2.2));   // Gamma校正
    
    fragColor = vec4(color, baseColor.a);
}

#### 3.2.4 模型加载器 (AdModelLoader)

```cpp
// Core/private/Resource/AdModelLoader.cpp
#include "Resource/AdModelResource.h"
#include "Render/AdVKDevice.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace WuDu {

bool AdModelLoader::LoadModel(const std::string& filePath, 
                             std::vector<ModelMesh>& meshes, 
                             std::vector<ModelMaterial>& materials, 
                             AdVKDevice* device) {
    Assimp::Importer importer;
    
    // 设置Assimp导入选项
    unsigned int importFlags = 
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_OptimizeMeshes;

    const aiScene* scene = importer.ReadFile(filePath, importFlags);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOG_E("Assimp error: {0}", importer.GetErrorString());
        return false;
    }

    // 处理材质
    ProcessMaterials(scene, materials);
    
    // 递归处理节点
    ProcessNode(scene->mRootNode, scene, meshes, materials, device);
    
    return true;
}

void AdModelLoader::ProcessNode(aiNode* node, const aiScene* scene, 
                               std::vector<ModelMesh>& meshes, 
                               std::vector<ModelMaterial>& materials, 
                               AdVKDevice* device) {
    // 处理当前节点的所有网格
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(ProcessMesh(mesh, scene, materials, device));
    }
    
    // 递归处理子节点
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene, meshes, materials, device);
    }
}

ModelMesh AdModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene, 
                                    std::vector<ModelMaterial>& materials, 
                                    AdVKDevice* device) {
    ModelMesh result;
    result.Name = mesh->mName.C_Str();
    result.MaterialIndex = mesh->mMaterialIndex;
    
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> indices;
    
    // 处理顶点数据
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
        } else {
            vertex.TexCoord = glm::vec2(0.0f, 0.0f);
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
    
    // 处理索引数据
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }
    
    // 创建顶点缓冲区
    result.VertexBuffer = std::make_shared<AdVKBuffer>(
        device,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vertices.size() * sizeof(ModelVertex),
        const_cast<void*>(static_cast<const void*>(vertices.data()))
    );
    
    // 创建索引缓冲区
    result.IndexBuffer = std::make_shared<AdVKBuffer>(
        device,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        indices.size() * sizeof(uint32_t),
        const_cast<void*>(static_cast<const void*>(indices.data()))
    );
    
    result.VertexCount = static_cast<uint32_t>(vertices.size());
    result.IndexCount = static_cast<uint32_t>(indices.size());
    
    return result;
}

void AdModelLoader::ProcessMaterials(const aiScene* scene, 
                                    std::vector<ModelMaterial>& materials) {
    materials.reserve(scene->mNumMaterials);
    
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* material = scene->mMaterials[i];
        ModelMaterial newMaterial;
        
        // 材质名称
        aiString name;
        if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
            newMaterial.Name = name.C_Str();
        } else {
            newMaterial.Name = "Material" + std::to_string(i);
        }
        
        // 漫反射颜色
        aiColor3D color(0.0f, 0.0f, 0.0f);
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            newMaterial.DiffuseColor = glm::vec3(color.r, color.g, color.b);
        } else {
            newMaterial.DiffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
        }
        
        // 镜面反射颜色
        if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
            newMaterial.SpecularColor = glm::vec3(color.r, color.g, color.b);
        } else {
            newMaterial.SpecularColor = glm::vec3(1.0f, 1.0f, 1.0f);
        }
        
        // 光泽度
        float shininess = 32.0f;
        if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
            newMaterial.Shininess = shininess;
        }
        
        // 漫反射纹理
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString texturePath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
                newMaterial.DiffuseTexturePath = texturePath.C_Str();
            }
        }
        
        // 法线纹理
        if (material->GetTextureCount(aiTextureType_NORMALS) > 0) {
            aiString texturePath;
            if (material->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS) {
                newMaterial.NormalTexturePath = texturePath.C_Str();
            }
        }
        
        // 镜面反射纹理
        if (material->GetTextureCount(aiTextureType_SPECULAR) > 0) {
            aiString texturePath;
            if (material->GetTexture(aiTextureType_SPECULAR, 0, &texturePath) == AI_SUCCESS) {
                newMaterial.SpecularTexturePath = texturePath.C_Str();
            }
        }
        
        materials.push_back(newMaterial);
    }
}

} // namespace WuDu
```

#### 3.2.5 模型资源实现 (AdModelResource)

```cpp
// Core/private/Resource/AdModelResource.cpp
#include "Resource/AdModelResource.h"
#include "Render/AdVKDevice.h"
#include "AdResourceManager.h"
#include "Adlog.h"

namespace WuDu {

AdModelResource::AdModelResource(const std::string& modelPath)
    : AdResource(modelPath) {
    mDevice = AdResourceManager::GetInstance()->GetDevice();
}

AdModelResource::~AdModelResource() {
    Unload();
}

bool AdModelResource::Load() {
    if (mState == ResourceState::Loaded) {
        return true;
    }
    
    mState = ResourceState::Loading;
    
    try {
        AdModelLoader loader;
        std::string fullPath = AdResourceManager::GetInstance()->GetFullPath(mPath);
        
        if (!loader.LoadModel(fullPath, mMeshes, mMaterials, mDevice.get())) {
            LOG_E("Failed to load model: {0}", mPath);
            mState = ResourceState::Failed;
            return false;
        }
        
        mState = ResourceState::Loaded;
        LOG_I("Successfully loaded model: {0}", mPath);
        return true;
    } catch (const std::exception& e) {
        LOG_E("Exception loading model {0}: {1}", mPath, e.what());
        mState = ResourceState::Failed;
        return false;
    }
}

void AdModelResource::Unload() {
    if (mState == ResourceState::Unloaded) {
        return;
    }
    
    mMeshes.clear();
    mMaterials.clear();
    mState = ResourceState::Unloaded;
    
    LOG_I("Unloaded model: {0}", mPath);
}

} // namespace WuDu
```

## 4. 模型渲染流程

### 4.1 渲染组件设计

#### 4.1.1 模型组件 (AdModelComponent)

```cpp
// Core/public/ECS/AdModelComponent.h
#ifndef AD_MODEL_COMPONENT_H
#define AD_MODEL_COMPONENT_H

#include "AdComponent.h"
#include "Resource/AdModelResource.h"
#include <memory>

namespace WuDu {

class AdModelComponent : public AdComponent {
public:
    AdModelComponent();
    ~AdModelComponent();
    
    void SetModel(const std::shared_ptr<AdModelResource>& model) {
        mModel = model;
    }
    
    const std::shared_ptr<AdModelResource>& GetModel() const {
        return mModel;
    }
    
    bool LoadModel(const std::string& modelPath);
    
    void SetVisible(bool visible) { mVisible = visible; }
    bool IsVisible() const { return mVisible;
    }
    
    void SetCastShadows(bool castShadows) { mCastShadows = castShadows; }
    bool CastShadows() const { return mCastShadows;
    }
    
    void SetReceiveShadows(bool receiveShadows) { mReceiveShadows = receiveShadows; }
    bool ReceiveShadows() const { return mReceiveShadows;
    }

private:
    std::shared_ptr<AdModelResource> mModel;
    bool mVisible;
    bool mCastShadows;
    bool mReceiveShadows;
};

} // namespace WuDu

#endif // AD_MODEL_COMPONENT_H
```

### 4.2 渲染系统实现

#### 4.2.1 模型渲染器 (AdModelRenderer)

```cpp
// Core/public/Render/AdModelRenderer.h
#ifndef AD_MODEL_RENDERER_H
#define AD_MODEL_RENDERER_H

#include "AdRenderer.h"
#include "ECS/AdEntity.h"
#include <vector>

namespace WuDu {

class AdModelRenderer {
public:
    AdModelRenderer(AdRenderContext* renderContext);
    ~AdModelRenderer();
    
    void Initialize();
    void Render(const std::vector<AdEntity>& visibleEntities, VkCommandBuffer commandBuffer);
    void Cleanup();
    
private:
    AdRenderContext* mRenderContext;
    VkPipelineLayout mPipelineLayout;
    VkPipeline mGraphicsPipeline;
    
    void CreatePipelines();
    void RenderModel(const AdModelResource& model, VkCommandBuffer commandBuffer, const glm::mat4& transform);
};

} // namespace WuDu

#endif // AD_MODEL_RENDERER_H
```

```cpp
// Core/private/Render/AdModelRenderer.cpp
#include "Render/AdModelRenderer.h"
#include "Render/AdVKDevice.h"
#include "Render/AdVKRenderPass.h"
#include "Render/AdVKPipeline.h"
#include "Render/AdVKPipelineLayout.h"
#include "ECS/AdModelComponent.h"
#include "ECS/AdTransformComponent.h"
#include "AdResourceManager.h"

namespace WuDu {

AdModelRenderer::AdModelRenderer(AdRenderContext* renderContext)
    : mRenderContext(renderContext),
      mPipelineLayout(nullptr),
      mGraphicsPipeline(nullptr) {
}

AdModelRenderer::~AdModelRenderer() {
    Cleanup();
}

void AdModelRenderer::Initialize() {
    CreatePipelines();
}

void AdModelRenderer::CreatePipelines() {
    auto device = mRenderContext->GetDevice();
    auto renderPass = /* 获取当前渲染通道 */;
    
    // 创建着色器布局
    ShaderLayout shaderLayout = {
        .pushConstants = {
            {
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(glm::mat4)
            }
        }
    };
    
    // 创建管线布局
    auto pipelineLayout = std::make_shared<AdVKPipelineLayout>(
        device,
        AD_RES_SHADER_DIR "03_unlit_material.vert",
        AD_RES_SHADER_DIR "03_unlit_material.frag",
        shaderLayout
    );
    
    mPipelineLayout = pipelineLayout->GetHandle();
    
    // 创建图形管线
    auto pipeline = std::make_shared<AdVKPipeline>(device, renderPass, pipelineLayout.get());
    
    // 配置管线状态
    std::vector<VkVertexInputBindingDescription> vertexBindings = {
        {
            0,
            sizeof(ModelVertex),
            VK_VERTEX_INPUT_RATE_VERTEX
        }
    };
    
    std::vector<VkVertexInputAttributeDescription> vertexAttrs = {
        {
            0,
            0,
            VK_FORMAT_R32G32B32_SFLOAT,
            offsetof(ModelVertex, Position)
        },
        {
            1,
            0,
            VK_FORMAT_R32G32B32_SFLOAT,
            offsetof(ModelVertex, Normal)
        },
        {
            2,
            0,
            VK_FORMAT_R32G32_SFLOAT,
            offsetof(ModelVertex, TexCoord)
        }
    };
    
    pipeline->SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        ->EnableDepthTest()
        ->SetDynamicState({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
        ->SetVertexInputState(vertexBindings, vertexAttrs)
        ->Create();
    
    mGraphicsPipeline = pipeline->GetHandle();
}

void AdModelRenderer::Render(const std::vector<AdEntity>& visibleEntities, VkCommandBuffer commandBuffer) {
    // 绑定图形管线
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);
    
    for (const auto& entity : visibleEntities) {
        // 获取模型组件
        auto modelComponent = entity.GetComponent<AdModelComponent>();
        if (!modelComponent || !modelComponent->IsVisible() || !modelComponent->GetModel()) {
            continue;
        }
        
        // 获取变换组件
        auto transformComponent = entity.GetComponent<AdTransformComponent>();
        if (!transformComponent) {
            continue;
        }
        
        // 获取模型资源
        const auto& model = modelComponent->GetModel();
        if (!model->IsLoaded()) {
            continue;
        }
        
        // 渲染模型
        RenderModel(*model, commandBuffer, transformComponent->GetWorldMatrix());
    }
}

void AdModelRenderer::RenderModel(const AdModelResource& model, VkCommandBuffer commandBuffer, const glm::mat4& transform) {
    // 将变换矩阵作为推送常量传递
    vkCmdPushConstants(
        commandBuffer,
        mPipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(glm::mat4),
        &transform[0][0]
    );
    
    // 渲染每个网格
    for (const auto& mesh : model.GetMeshes()) {
        // 1. 绑定材质
        if (mesh.MaterialIndex < model.GetMaterials().size()) {
            const auto& material = model.GetMaterials()[mesh.MaterialIndex];
            BindMaterial(material, commandBuffer);
        }
        
        // 2. 绑定顶点缓冲区
        VkBuffer vertexBuffers[] = { mesh.VertexBuffer->GetHandle() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        
        // 3. 绑定索引缓冲区
        vkCmdBindIndexBuffer(
            commandBuffer,
            mesh.IndexBuffer->GetHandle(),
            0,
            VK_INDEX_TYPE_UINT32
        );
        
        // 4. 执行绘制命令
        vkCmdDrawIndexed(commandBuffer, mesh.IndexCount, 1, 0, 0, 0);
    }
}

void AdModelRenderer::Cleanup() {
    // 清理资源
    // 注意：实际实现中需要正确管理管线和布局的生命周期
}

} // namespace WuDu
```

## 5. 资源系统集成

### 5.1 与现有渲染系统集成

#### 5.1.1 更新AdRenderer类

```cpp
// Core/private/Render/AdRenderer.cpp
#include "Render/AdModelRenderer.h"

// 在AdRenderer类中添加模型渲染器
std::unique_ptr<AdModelRenderer> mModelRenderer;

// 在Initialize方法中初始化
mModelRenderer = std::make_unique<AdModelRenderer>(mRenderContext);
mModelRenderer->Initialize();

// 在Render方法中调用模型渲染
mModelRenderer->Render(visibleEntities, commandBuffer);
```

### 5.2 与应用程序集成

#### 5.2.1 更新AdApplication类

```cpp
// Core/private/AdApplication.cpp
#include "Resource/AdResourceManager.h"

void AdApplication::OnInit() {
    // 初始化资源管理器
    auto resourceManager = AdResourceManager::GetInstance();
    resourceManager->SetResourceRootPath(AD_DEFINE_RES_ROOT_DIR);
    
    // 初始化其他组件
    // ...
}

void AdApplication::OnDestroy() {
    // 清理资源管理器
    auto resourceManager = AdResourceManager::GetInstance();
    resourceManager->UnloadAll();
    
    // 清理其他组件
    // ...
}
```

## 6. 使用示例

### 6.1 加载和渲染模型

```cpp
// Sample/ModelLoading/Main.cpp
#include "AdApplication.h"
#include "ECS/AdEntity.h"
#include "ECS/AdModelComponent.h"
#include "ECS/AdTransformComponent.h"
#include "Resource/AdResourceManager.h"

class ModelLoadingApp : public WuDu::AdApplication {
protected:
    void OnInit() override {
        // 创建实体
        auto& entity = mScene->CreateEntity();
        
        // 添加变换组件
        auto transform = entity.AddComponent<WuDu::AdTransformComponent>();
        transform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        transform->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
        transform->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
        
        // 添加模型组件并加载模型
        auto modelComponent = entity.AddComponent<WuDu::AdModelComponent>();
        bool success = modelComponent->LoadModel("Model/cube.obj");
        
        if (success) {
            ADE_LOG_INFO("Model loaded successfully!");
        } else {
            ADE_LOG_ERROR("Failed to load model!");
        }
    }
    
    void OnUpdate(float deltaTime) override {
        // 获取实体并旋转模型
        auto& entity = mScene->GetEntity(0);
        auto transform = entity.GetComponent<WuDu::AdTransformComponent>();
        
        if (transform) {
            glm::quat rotation = transform->GetRotation();
            rotation = glm::rotate(rotation, deltaTime * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
            transform->SetRotation(rotation);
        }
    }
};

int main(int argc, char* argv[]) {
    ModelLoadingApp app;
    app.Start(argc, argv);
    return 0;
}
```

### 6.2 手动加载模型

```cpp
// 手动加载模型的示例
#include "Resource/AdResourceManager.h"
#include "Resource/AdModelResource.h"

// 获取资源管理器实例
auto resourceManager = WuDu::AdResourceManager::GetInstance();

// 加载模型
std::shared_ptr<WuDu::AdModelResource> model = 
    resourceManager->Load<WuDu::AdModelResource>("Model/nanosuit.obj");

// 检查加载状态
if (model && model->IsLoaded()) {
    // 访问模型数据
    const auto& meshes = model->GetMeshes();
    const auto& materials = model->GetMaterials();
    
    std::cout << "Loaded model with " << meshes.size() << " meshes and " 
              << materials.size() << " materials." << std::endl;
}
```

## 7. 外部依赖

### 7.1 Assimp库

为了实现模型加载功能，我们需要集成Assimp库：

1. **下载Assimp**：从[Assimp官网](https://github.com/assimp/assimp)下载最新版本
2. **集成到项目**：
   - 在CMakeLists.txt中添加Assimp依赖
   - 配置包含目录和库目录

### 7.2 CMake配置

```cmake
# CMakeLists.txt (添加到Platform/CMakeLists.txt)

# Assimp配置
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)

add_subdirectory(External/assimp)

# 链接Assimp库
target_link_libraries(WuDu_platform PRIVATE assimp)
target_include_directories(WuDu_platform PRIVATE ${ASSIMP_INCLUDE_DIRS})
```

## 8. 优化和扩展

### 8.1 性能优化

1. **资源流式加载**：
   - 实现异步资源加载
   - 使用线程池处理资源加载

2. **内存优化**：
   - 实现资源压缩
   - 纹理压缩（ASTC、BCn等）
   - 几何数据压缩

3. **渲染优化**：
   - 实现实例化渲染
   - 批量绘制
   - 几何体合并

### 8.2 扩展功能

1. **支持更多模型格式**：
   - 通过Assimp支持多种3D模型格式
   - 实现自定义模型格式

2. **高级材质系统**：
   - PBR材质支持
   - 材质编辑器

3. **资源热重载**：
   - 支持运行时重新加载资源
   - 提高开发效率

## 9. 结论

本文档详细设计了WuDu Engine的资源系统，重点关注外部模型的导入和渲染流程。通过实现这个资源系统，引擎将能够：

1. 加载和管理各种3D模型文件
2. 高效地渲染复杂模型
3. 支持材质系统
4. 提供灵活的资源管理机制

这个资源系统的设计遵循了现代游戏引擎的最佳实践，包括资源缓存、引用计数、异步加载等技术，可以有效地支持游戏开发过程中的资源管理需求。

通过将这个资源系统集成到现有的渲染架构中，可以显著提升引擎的功能和性能，为开发者提供更强大的工具来创建高质量的3D应用和游戏。