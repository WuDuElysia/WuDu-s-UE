# Vulkan Mesh Shading 集成方案

## 1. 概述

本文档介绍如何在现有Vulkan渲染系统中集成`VK_NV_mesh_shader`扩展，实现基于mesh shading的渲染管线。Mesh shading是传统光栅化管线的顶点、细分和几何着色器阶段的替代方案，提供更灵活的几何处理能力。

## 2. 现有架构分析

### 2.1 核心渲染类

- **AdVKPipelineLayout**：管理着色器模块和管线布局
- **AdVKPipeline**：管理渲染管线状态
- **AdMesh**：管理顶点和索引缓冲区
- **AdUnlitMaterialSystem**：管理材质和渲染逻辑

### 2.2 当前渲染流程

```
1. 创建顶点/索引缓冲区 (AdMesh)
2. 配置顶点输入状态
3. 创建图形管线 (传统顶点-片段着色器)
4. 绑定管线和缓冲区
5. 执行绘制命令
```

## 3. Mesh Shading 集成方案

### 3.1 扩展核心类

#### 3.1.1 AdVKPipelineLayout 扩展

需要扩展`AdVKPipelineLayout`类以支持任务着色器(Task Shader)和网格着色器(Mesh Shader)：

```cpp
// 在AdVKPipelineLayout类中添加任务和网格着色器支持
class AdVKPipelineLayout {
public:
    // 新增构造函数，支持mesh shading
    AdVKPipelineLayout(AdVKDevice* device, 
                       const std::string& taskShaderFile,
                       const std::string& meshShaderFile, 
                       const std::string& fragShaderFile,
                       const ShaderLayout& shaderLayout = {});
    
    // 获取任务着色器模块
    VkShaderModule GetTaskShaderModule() const { return mTaskShaderModule; }
    
    // 获取网格着色器模块  
    VkShaderModule GetMeshShaderModule() const { return mMeshShaderModule; }
    
private:
    // 新增成员变量
    VkShaderModule mTaskShaderModule = VK_NULL_HANDLE;
    VkShaderModule mMeshShaderModule = VK_NULL_HANDLE;
};
```

#### 3.1.2 AdVKPipeline 扩展

需要扩展`AdVKPipeline`类以支持mesh shading特定的管线配置：

```cpp
// 在PipelineConfig中添加mesh shading支持
enum class PipelineType {
    GRAPHICS,  // 传统图形管线
    MESH       // Mesh shading管线
};

struct PipelineConfig {
    // 现有配置...
    PipelineType type = PipelineType::GRAPHICS;
    
    // Mesh shading特定配置
    struct MeshState {
        uint32_t maxTasks = 0;
        uint32_t maxMeshVertices = 0;
        uint32_t maxMeshPrimitives = 0;
    } meshState;
};

// 在AdVKPipeline类中添加mesh shading方法
class AdVKPipeline {
public:
    // 配置为mesh shading管线
    AdVKPipeline* SetMeshPipelineType(uint32_t maxTasks, 
                                     uint32_t maxMeshVertices, 
                                     uint32_t maxMeshPrimitives);
    
private:
    // 在Create()方法中根据PipelineType配置不同的管线阶段
    void CreateGraphicsPipeline();
    void CreateMeshPipeline();
};
```

### 3.2 创建Mesh Shading着色器

#### 3.2.1 任务着色器 (task_shader.glsl)

```glsl
#version 450
#extension GL_NV_mesh_shader : require

layout(local_size_x = 1) in;
layout(max_vertices = 3, max_primitives = 1) out;
layout(taskNV, invocations = 1) out;

void main() {
    // 生成一个任务，包含1个网格实例
    gl_TaskCountNV = 1;
    
    // 设置网格实例数据（可选）
    gl_TaskNV[0].gl_ViewMask = 0;
    gl_TaskNV[0].gl_ViewportIndex = 0;
    gl_TaskNV[0].gl_DrawID = 0;
}
```

#### 3.2.2 网格着色器 (mesh_shader.glsl)

```glsl
#version 450
#extension GL_NV_mesh_shader : require

layout(local_size_x = 3) in;
layout(triangles) out;
layout(max_vertices = 3, max_primitives = 1) out;

// 输出顶点属性
layout(location = 0) out vec3 outColor;

// 简单三角形的顶点数据
const vec3 vertices[3] = {
    vec3(-0.5, -0.5, 0.0),
    vec3(0.5, -0.5, 0.0),
    vec3(0.0, 0.5, 0.0)
};

const vec3 colors[3] = {
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
};

void main() {
    uint idx = gl_LocalInvocationID.x;
    
    // 生成顶点
    gl_MeshVerticesNV[idx].gl_Position = vec4(vertices[idx], 1.0);
    outColor = colors[idx];
    
    // 生成三角形图元
    if (idx == 0) {
        gl_PrimitiveCountNV = 1;
        gl_PrimitiveNV[0].gl_VertexIDNV[0] = 0;
        gl_PrimitiveNV[0].gl_VertexIDNV[1] = 1;
        gl_PrimitiveNV[0].gl_VertexIDNV[2] = 2;
    }
}
```

#### 3.2.3 片段着色器 (mesh_frag.glsl)

```glsl
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(inColor, 1.0);
}
```

### 3.3 渲染管线创建流程

```cpp
// 1. 创建支持mesh shading的管线布局
ShaderLayout shaderLayout;
// 配置描述符集和推送常量...

auto pipelineLayout = std::make_shared<AdVKPipelineLayout>(
    device,
    AD_RES_SHADER_DIR"task_shader",
    AD_RES_SHADER_DIR"mesh_shader",
    AD_RES_SHADER_DIR"mesh_frag",
    shaderLayout
);

// 2. 创建mesh shading管线
auto pipeline = std::make_shared<AdVKPipeline>(device, renderPass, pipelineLayout.get());
pipeline->SetMeshPipelineType(1, 3, 1); // maxTasks, maxMeshVertices, maxMeshPrimitives
pipeline->SetDynamicState({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR});
pipeline->SetMultisampleState(VK_SAMPLE_COUNT_4_BIT, VK_FALSE);
pipeline->SetSubPassIndex(0);
pipeline->Create();

// 3. 执行mesh shading渲染
vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
pipeline->Bind(cmdBuffer);

// 使用mesh shading绘制（替代传统的vkCmdDrawIndexed）
vkCmdDrawMeshTasksNV(cmdBuffer, 1, 1, 1);

vkCmdEndRenderPass(cmdBuffer);
```

### 3.4 与现有AdMesh的集成

可以扩展AdMesh类以支持mesh shading数据格式：

```cpp
class AdMesh {
public:
    // 现有方法...
    
    // 新增：创建用于mesh shading的缓冲区
    void CreateMeshShaderBuffers();
    
    // 新增：使用mesh shading绘制
    void DrawMeshShader(VkCommandBuffer cmdBuffer);
    
private:
    // 新增：用于mesh shading的任务和网格数据缓冲区
    std::shared_ptr<AdVKBuffer> mTaskBuffer;
    std::shared_ptr<AdVKBuffer> mMeshDataBuffer;
};
```

## 4. 实现步骤

### 4.1 第一步：检查扩展支持

在设备初始化时检查`VK_NV_mesh_shader`扩展支持：

```cpp
bool IsMeshShaderSupported(AdVKDevice* device) {
    // 检查物理设备扩展
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device->GetPhysicalDevice(), nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device->GetPhysicalDevice(), nullptr, &extensionCount, extensions.data());
    
    for (const auto& extension : extensions) {
        if (strcmp(extension.extensionName, VK_NV_MESH_SHADER_EXTENSION_NAME) == 0) {
            return true;
        }
    }
    return false;
}
```

### 4.2 第二步：扩展AdVKPipelineLayout

修改AdVKPipelineLayout.h和.cpp文件，添加任务和网格着色器支持。

### 4.3 第三步：扩展AdVKPipeline

修改AdVKPipeline.h和.cpp文件，添加mesh shading管线配置。

### 4.4 第四步：创建着色器文件

在Resource/Shader目录下创建task_shader.glsl、mesh_shader.glsl和mesh_frag.glsl文件。

### 4.5 第五步：编译着色器

使用glslc编译着色器到SPIR-V格式：

```bash
glslc task_shader.glsl -o task_shader.vert.spv -e main -D VK_NV_mesh_shader
glslc mesh_shader.glsl -o mesh_shader.vert.spv -e main -D VK_NV_mesh_shader
glslc mesh_frag.glsl -o mesh_frag.frag.spv -e main
```

### 4.6 第六步：创建示例应用

在Sample目录下创建一个新的示例应用，展示mesh shading的使用。

## 5. 性能考虑

1. **任务粒度**：合理设置任务数量和每个任务处理的几何体量
2. **内存访问**：优化mesh shader中的内存访问模式，提高缓存命中率
3. **并行度**：充分利用GPU并行处理能力，避免线程发散
4. **数据压缩**：考虑对输入数据进行压缩，减少内存带宽需求

## 6. 兼容性

- `VK_NV_mesh_shader`是NVIDIA的扩展，目前主要支持NVIDIA GPU
- 跨平台支持可以考虑`VK_EXT_mesh_shader`扩展（如果可用）
- 建议实现回退机制，在不支持mesh shading的设备上使用传统渲染管线

## 7. 总结

集成mesh shading可以为渲染系统提供更灵活的几何处理能力，特别适合大规模并行场景。通过扩展现有渲染类并创建相应的着色器，可以在现有架构基础上平滑集成mesh shading功能。