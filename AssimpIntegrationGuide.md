# Assimp集成与模型导入详细指南

本文档将详细讲解如何集成Assimp库到您的项目中，并实现"从Assimp导入的模型中提取原始顶点数据"的功能（对应ResourceSystemDocumentation.md第377行）。

## 1. Assimp库概述

Assimp（Open Asset Import Library）是一个强大的3D模型导入库，支持40多种3D文件格式，包括：
- OBJ、FBX、Collada (DAE)
- STL、GLTF、3DS
- Blender (BLEND)、Maya (MA)、3D Max (MAX)

## 2. 下载与安装Assimp

### 2.1 从GitHub下载

```bash
# 克隆Assimp仓库（推荐使用v5.2.5稳定版）
git clone --branch v5.2.5 https://github.com/assimp/assimp.git

# 进入Assimp目录
cd assimp

# 初始化子模块
git submodule update --init --recursive
```

### 2.2 编译Assimp

**使用CMake编译：**

```bash
# 创建构建目录
mkdir build
cd build

# 配置CMake（Windows示例）
cmake .. -G "Visual Studio 16 2019" -A x64 \
    -DASSIMP_BUILD_ASSIMP_TOOLS=OFF \
    -DASSIMP_BUILD_TESTS=OFF \
    -DASSIMP_INSTALL=OFF \
    -DASSIMP_NO_EXPORT=ON \
    -DASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=OFF \
    -DASSIMP_BUILD_OBJ_IMPORTER=ON \
    -DASSIMP_BUILD_FBX_IMPORTER=ON \
    -DASSIMP_BUILD_GLTF_IMPORTER=ON

# 编译
cmake --build . --config Release
```

## 3. 集成Assimp到项目

### 3.1 复制Assimp到项目

将编译好的Assimp库复制到您的项目中：

```
Platform/External/assimp/
├── include/          # Assimp头文件
├── lib/              # 编译好的库文件
│   ├── Release/      # Release版本库
│   └── Debug/        # Debug版本库
└── CMakeLists.txt    # Assimp的CMake配置
```

### 3.2 更新项目CMake配置

修改`Platform/CMakeLists.txt`，添加Assimp依赖：

```cmake
# Assimp配置
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)

# 添加Assimp子目录
add_subdirectory(External/assimp)

# 链接Assimp库
target_link_libraries(WuDu_platform PRIVATE assimp)
target_include_directories(WuDu_platform PRIVATE ${ASSIMP_INCLUDE_DIRS})
```

## 4. 实现模型加载系统

### 4.1 定义模型数据结构

首先，我们需要定义与Assimp兼容的模型数据结构：

**创建`Core/public/Resource/AdModelResource.h`：**

```cpp
#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace WuDu {

// 模型顶点结构
struct ModelVertex {
    glm::vec3 Position;
    glm::vec2 TexCoord;
    glm::vec3 Normal;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

// 模型材质结构
struct ModelMaterial {
    glm::vec3 BaseColor;
    float Metallic;
    float Roughness;
    float AO;
    float Alpha;             // 透明度
    glm::vec3 EmissiveColor; // 自发光颜色
    float EmissiveStrength;  // 自发光强度
    float Anisotropy;        // 各向异性因子(0.0=无各向异性, 1.0=完全各向异性)
    glm::vec3 AnisotropyDirection; // 各向异性方向
    std::string BaseColorTexturePath;
    std::string NormalTexturePath;
    std::string MetallicRoughnessTexturePath;
};

// 模型网格结构
struct ModelMesh {
    std::vector<ModelVertex> Vertices;
    std::vector<uint32_t> Indices;
    uint32_t MaterialIndex;
};

// 模型资源类
class AdModelResource {
public:
    AdModelResource(const std::string& modelPath);
    ~AdModelResource();

    bool Load();
    void Unload();

    const std::vector<ModelMesh>& GetMeshes() const { return mMeshes; }
    const std::vector<ModelMaterial>& GetMaterials() const { return mMaterials; }
    bool IsLoaded() const { return mIsLoaded; }

private:
    std::string mModelPath;
    std::vector<ModelMesh> mMeshes;
    std::vector<ModelMaterial> mMaterials;
    bool mIsLoaded = false;
};

} // namespace WuDu
```

### 4.2 实现模型加载器

**创建`Core/private/Resource/AdModelLoader.cpp`：**

```cpp
#include "Resource/AdModelResource.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace WuDu {

class AdModelLoader {
public:
    static bool LoadModel(const std::string& filePath, 
                         std::vector<ModelMesh>& meshes, 
                         std::vector<ModelMaterial>& materials) {
        Assimp::Importer importer;
        
        // 设置Assimp导入选项
        unsigned int importFlags = 
            aiProcess_Triangulate |          // 将所有图元转换为三角形
            aiProcess_FlipUVs |              // 翻转纹理坐标Y轴
            aiProcess_CalcTangentSpace |     // 计算切线和副切线
            aiProcess_GenSmoothNormals |     // 生成平滑法线
            aiProcess_JoinIdenticalVertices |// 合并相同顶点
            aiProcess_OptimizeMeshes;        // 优化网格

        // 导入模型
        const aiScene* scene = importer.ReadFile(filePath, importFlags);
        
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            LOG_E("Assimp error: {0}", importer.GetErrorString());
            return false;
        }

        // 处理材质
        ProcessMaterials(scene, materials);
        
        // 递归处理所有节点
        ProcessNode(scene->mRootNode, scene, meshes, materials);
        
        return true;
    }

private:
    // 处理Assimp节点
    static void ProcessNode(aiNode* node, const aiScene* scene, 
                           std::vector<ModelMesh>& meshes, 
                           std::vector<ModelMaterial>& materials) {
        // 处理当前节点的所有网格
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(ProcessMesh(mesh, scene, materials));
        }

        // 递归处理子节点
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], scene, meshes, materials);
        }
    }

    // 处理单个网格（重点：提取顶点数据）
    static ModelMesh ProcessMesh(aiMesh* mesh, const aiScene* scene, 
                                const std::vector<ModelMaterial>& materials) {
        ModelMesh result;
        result.MaterialIndex = mesh->mMaterialIndex;

        // 提取顶点数据（对应文档第377行）
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            ModelVertex vertex;
            
            // 提取位置
            vertex.Position.x = mesh->mVertices[i].x;
            vertex.Position.y = mesh->mVertices[i].y;
            vertex.Position.z = mesh->mVertices[i].z;
            
            // 提取纹理坐标（如果存在）
            if (mesh->mTextureCoords[0]) {
                vertex.TexCoord.x = mesh->mTextureCoords[0][i].x;
                vertex.TexCoord.y = mesh->mTextureCoords[0][i].y;
            } else {
                vertex.TexCoord = glm::vec2(0.0f, 0.0f);
            }
            
            // 提取法线
            vertex.Normal.x = mesh->mNormals[i].x;
            vertex.Normal.y = mesh->mNormals[i].y;
            vertex.Normal.z = mesh->mNormals[i].z;
            
            // 提取切线和副切线（如果存在）
            if (mesh->mTangents && mesh->mBitangents) {
                vertex.Tangent.x = mesh->mTangents[i].x;
                vertex.Tangent.y = mesh->mTangents[i].y;
                vertex.Tangent.z = mesh->mTangents[i].z;
                
                vertex.Bitangent.x = mesh->mBitangents[i].x;
                vertex.Bitangent.y = mesh->mBitangents[i].y;
                vertex.Bitangent.z = mesh->mBitangents[i].z;
            }
            
            result.Vertices.push_back(vertex);
        }

        // 提取索引数据
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                result.Indices.push_back(face.mIndices[j]);
            }
        }

        return result;
    }

    // 处理材质
    static void ProcessMaterials(const aiScene* scene, 
                               std::vector<ModelMaterial>& materials) {
        materials.reserve(scene->mNumMaterials);
        
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* material = scene->mMaterials[i];
            ModelMaterial mat;
            
            // 提取PBR基础颜色
            aiColor3D baseColor(0.0f, 0.0f, 0.0f);
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == AI_SUCCESS) {
                mat.BaseColor = glm::vec3(baseColor.r, baseColor.g, baseColor.b);
            } else {
                mat.BaseColor = glm::vec3(1.0f, 1.0f, 1.0f);  // 默认白色
            }
            
            // 提取透明度
            float alpha = 1.0f;
            if (material->Get(AI_MATKEY_OPACITY, alpha) == AI_SUCCESS) {
                mat.Alpha = alpha;
            } else {
                mat.Alpha = 1.0f;  // 默认完全不透明
            }
            
            // 提取自发光颜色
            aiColor3D emissiveColor(0.0f, 0.0f, 0.0f);
            if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == AI_SUCCESS) {
                mat.EmissiveColor = glm::vec3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
            } else {
                mat.EmissiveColor = glm::vec3(0.0f, 0.0f, 0.0f);  // 默认不发光
            }
            
            // 提取PBR金属度
            float metallic = 0.0f;
            if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
                mat.Metallic = metallic;
            } else if (material->Get("$mat.metallic", 0, 0, metallic) == AI_SUCCESS) {
                mat.Metallic = metallic;
            } else {
                mat.Metallic = 0.0f;      // 默认非金属
            }
            
            // 提取PBR粗糙度
            float roughness = 0.5f;
            if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
                mat.Roughness = roughness;
            } else if (material->Get("$mat.roughness", 0, 0, roughness) == AI_SUCCESS) {
                mat.Roughness = roughness;
            } else {
                mat.Roughness = 0.5f;     // 默认中等粗糙度
            }
            
            // 提取环境光遮蔽
            float ao = 1.0f;
            if (material->Get(AI_MATKEY_OCCLUSION_STRENGTH, ao) == AI_SUCCESS) {
                mat.AO = ao;
            } else if (material->Get("$mat.aoStrength", 0, 0, ao) == AI_SUCCESS) {
                mat.AO = ao;
            } else {
                mat.AO = 1.0f;            // 默认完全环境光遮蔽
            }
            
            // 设置各向异性默认值
            // 说明: 各向异性属性通常不在模型文件中定义，需要用户手动调整
            // 各向异性因子: 0.0表示无各向异性，1.0表示完全各向异性
            // 各向异性方向: 定义了材料表面的方向，影响高光的形状
            mat.Anisotropy = 0.0f;           // 默认无各向异性
            mat.AnisotropyDirection = glm::vec3(1.0f, 0.0f, 0.0f); // 默认各向异性方向
            
            // 提取自发光强度
            float emissiveStrength = 1.0f;
            // 尝试从自发光颜色的强度推断，或直接提取
            aiColor3D emissive;            
            if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS) {
                emissiveStrength = glm::length(glm::vec3(emissive.r, emissive.g, emissive.b));
            } else if (material->Get("$mat.emissiveStrength", 0, 0, emissiveStrength) == AI_SUCCESS) {
                // 某些格式可能有专门的自发光强度属性
            }
            mat.EmissiveStrength = emissiveStrength;
            

            
            // 提取PBR纹理路径
            aiString texturePath;
            
            // 设置基础颜色纹理
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
                mat.BaseColorTexturePath = texturePath.C_Str();
            }
            
            // 设置法线纹理
            if (material->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS) {
                mat.NormalTexturePath = texturePath.C_Str();
            }
            
            // 设置金属度纹理
            if (material->GetTexture(aiTextureType_METALNESS, 0, &texturePath) == AI_SUCCESS) {
                mat.MetallicTexturePath = texturePath.C_Str();
            }
            
            // 设置粗糙度纹理
            if (material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texturePath) == AI_SUCCESS) {
                mat.RoughnessTexturePath = texturePath.C_Str();
            }
            
            // 设置环境光遮蔽纹理
            if (material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &texturePath) == AI_SUCCESS) {
                mat.AOTexturePath = texturePath.C_Str();
            }
            

            
            // 设置透明度贴图
            if (material->GetTexture(aiTextureType_OPACITY, 0, &texturePath) == AI_SUCCESS) {
                mat.AlphaTexturePath = texturePath.C_Str();
            }
            
            // 设置自发光纹理
            if (material->GetTexture(aiTextureType_EMISSIVE, 0, &texturePath) == AI_SUCCESS) {
                mat.EmissiveTexturePath = texturePath.C_Str();
            }
        
        // 设置法线纹理
        if (material->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS) {
            mat.NormalTexturePath = texturePath.C_Str();
        }
            
            materials.push_back(mat);
        }
    }
};

} // namespace WuDu
```

### 4.3 实现模型资源类

**更新`Core/private/Resource/AdModelResource.cpp`：**

```cpp
#include "Resource/AdModelResource.h"
#include "AdModelLoader.h"
#include "Adlog.h"

namespace WuDu {

AdModelResource::AdModelResource(const std::string& modelPath)
    : mModelPath(modelPath) {
}

AdModelResource::~AdModelResource() {
    Unload();
}

bool AdModelResource::Load() {
    if (mIsLoaded) {
        return true;
    }
    
    LOG_I("Loading model: {0}", mModelPath);
    
    if (AdModelLoader::LoadModel(mModelPath, mMeshes, mMaterials)) {
        mIsLoaded = true;
        LOG_I("Model loaded successfully. Meshes: {0}, Materials: {1}", 
              mMeshes.size(), mMaterials.size());
        return true;
    } else {
        LOG_E("Failed to load model: {0}", mModelPath);
        return false;
    }
}

void AdModelResource::Unload() {
    if (mIsLoaded) {
        mMeshes.clear();
        mMaterials.clear();
        mIsLoaded = false;
        LOG_I("Model unloaded: {0}", mModelPath);
    }
}

} // namespace WuDu
```

## 5. 与渲染系统集成

### 5.1 创建顶点缓冲区

将Assimp加载的顶点数据转换为Vulkan顶点缓冲区：

```cpp
// 假设我们有一个ModelMesh对象
ModelMesh mesh = ...;

// 创建顶点缓冲区
VkBufferCreateInfo bufferInfo = {};
bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
bufferInfo.size = mesh.Vertices.size() * sizeof(ModelVertex);
bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

VkBuffer vertexBuffer;
vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer);

// 分配内存并复制数据
VkMemoryRequirements memRequirements;
vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

VkMemoryAllocateInfo allocInfo = {};
allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
allocInfo.allocationSize = memRequirements.size;
allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, 
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

VkDeviceMemory vertexBufferMemory;
vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory);
vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

// 复制顶点数据
void* data;
vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
memcpy(data, mesh.Vertices.data(), (size_t)bufferInfo.size);
vkUnmapMemory(device, vertexBufferMemory);
```

### 5.2 设置顶点输入布局

```cpp
// 顶点绑定描述
VkVertexInputBindingDescription bindingDescription = {};
bindingDescription.binding = 0;
bindingDescription.stride = sizeof(ModelVertex);
bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

// 顶点属性描述
std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

// 位置属性
VkVertexInputAttributeDescription posAttr = {};
posAttr.binding = 0;
posAttr.location = 0;
posAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
posAttr.offset = offsetof(ModelVertex, Position);
attributeDescriptions.push_back(posAttr);

// 纹理坐标属性
VkVertexInputAttributeDescription texAttr = {};
texAttr.binding = 0;
texAttr.location = 1;
texAttr.format = VK_FORMAT_R32G32_SFLOAT;
texAttr.offset = offsetof(ModelVertex, TexCoord);
attributeDescriptions.push_back(texAttr);

// 法线属性
VkVertexInputAttributeDescription normAttr = {};
normAttr.binding = 0;
normAttr.location = 2;
normAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
normAttr.offset = offsetof(ModelVertex, Normal);
attributeDescriptions.push_back(normAttr);

// 切线属性
VkVertexInputAttributeDescription tangentAttr = {};
tangentAttr.binding = 0;
tangentAttr.location = 3;
tangentAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
tangentAttr.offset = offsetof(ModelVertex, Tangent);
attributeDescriptions.push_back(tangentAttr);

// 副切线属性
VkVertexInputAttributeDescription bitangentAttr = {};
bitangentAttr.binding = 0;
bitangentAttr.location = 4;
bitangentAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
bitangentAttr.offset = offsetof(ModelVertex, Bitangent);
attributeDescriptions.push_back(bitangentAttr);
```

## 6. 使用示例

### 6.1 加载模型

```cpp
#include "Resource/AdModelResource.h"

// 加载模型
std::shared_ptr<AdModelResource> model = std::make_shared<AdModelResource>("Resource/Model/nanosuit.obj");
if (model->Load()) {
    // 获取模型数据
    const auto& meshes = model->GetMeshes();
    const auto& materials = model->GetMaterials();
    
    // 处理每个网格
    for (const auto& mesh : meshes) {
        // 创建顶点缓冲区和索引缓冲区
        // 设置渲染管线
        // 绘制网格
    }
}
```

### 6.2 渲染模型

```cpp
// 绑定顶点缓冲区
VkBuffer vertexBuffers[] = {vertexBuffer};
VkDeviceSize offsets[] = {0};
vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

// 绑定索引缓冲区
vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

// 设置材质
// ...

// 绘制索引
vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
```

## 7. 常见问题与解决方案

### 7.1 纹理路径问题

Assimp返回的纹理路径可能是相对路径，需要转换为绝对路径：

```cpp
std::string GetAbsolutePath(const std::string& modelPath, const std::string& texturePath) {
    if (std::filesystem::path(texturePath).is_absolute()) {
        return texturePath;
    }
    
    std::filesystem::path modelDir = std::filesystem::path(modelPath).parent_path();
    return (modelDir / texturePath).string();
}
```

### 7.2 模型缩放问题

某些模型可能需要缩放才能在场景中正确显示：

```cpp
// 统一缩放因子
float scaleFactor = 0.1f;
for (auto& vertex : mesh.Vertices) {
    vertex.Position *= scaleFactor;
}
```

### 7.3 内存优化

对于大型模型，可以考虑：
- 使用索引缓冲区减少重复顶点
- 合并相似网格减少绘制调用
- 使用LOD（Level of Detail）技术

## 8. 总结

通过以上步骤，您已经成功集成了Assimp库并实现了模型加载功能。现在，您可以：

1. 从各种3D格式加载模型
2. 提取顶点数据（位置、纹理坐标、法线等）
3. 将模型数据转换为Vulkan可使用的格式
4. 在场景中渲染加载的模型

这就是ResourceSystemDocumentation.md第377行提到的"从Assimp导入的模型中提取原始顶点数据"的完整实现过程。

## 9. 后续扩展

完成基础集成后，您可以考虑：

- 实现资源管理器统一管理模型资源
- 添加PBR材质支持
- 实现动画模型加载
- 添加模型优化功能（如网格简化）
- 实现模型编辑器