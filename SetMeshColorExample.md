# 设置Mesh颜色为白色的示例

根据查看的代码分析，您的理解是**完全正确**的：

1. 当前顶点结构（`AdVertex`）**确实没有颜色属性**
2. Mesh的颜色是通过**材质系统的UBO参数**传递的
3. 您可以直接设置`baseColor`为白色，使整个mesh显示为白色

## 当前着色器的颜色处理逻辑

在`03_unlit_material.frag`中，颜色计算逻辑如下：

```glsl
// 从材质UBO获取颜色
vec3 color0 = materialUbo.baseColor0;
vec3 color1 = materialUbo.baseColor1;

// 如果启用纹理，则用纹理颜色覆盖baseColor
if(materialUbo.textureParam0.enable) {
    color0 = texture(texture0, getTextureUV(param, v_Texcoord)).rgb;
}

if(materialUbo.textureParam1.enable) {
    color1 = texture(texture1, getTextureUV(materialUbo.textureParam1, v_Texcoord)).rgb;
}

// 混合两种颜色并输出
fragColor = vec4(mix(color0, color1, materialUbo.mixValue), 1.0);
```

## 设置Mesh为白色的代码示例

### 方法1：禁用纹理，直接使用baseColor

```cpp
// 假设您有一个AdUnlitMaterial对象
AdUnlitMaterial* material = new AdUnlitMaterial();

// 设置baseColor0为白色
material->SetBaseColor0(glm::vec3(1.0f, 1.0f, 1.0f)); // RGB白色

// 设置baseColor1也为白色（可选，但为了一致性）
material->SetBaseColor1(glm::vec3(1.0f, 1.0f, 1.0f));

// 设置mixValue为0（只使用baseColor0）或1（只使用baseColor1）
material->SetMixValue(0.0f);

// 确保纹理被禁用（如果之前启用了的话）
material->SetTextureEnable(0, false);
material->SetTextureEnable(1, false);

// 将材质应用到mesh
// ...
```

### 方法2：启用纹理但使用白色纹理

如果您需要保留纹理功能但希望显示为白色，可以创建一个纯白色的纹理并应用：

```cpp
// 创建白色纹理
// 这里假设您有创建纹理的工具函数
Texture* whiteTexture = CreateWhiteTexture();

// 应用到材质
material->SetTexture(0, whiteTexture);
material->SetTextureEnable(0, true);

// 设置baseColor为白色（可选，纹理会覆盖它）
material->SetBaseColor0(glm::vec3(1.0f, 1.0f, 1.0f));
```

## 颜色混合说明

当前材质系统支持两种颜色（baseColor0和baseColor1）的混合：
- 当`mixValue = 0.0f`时，完全使用`baseColor0`
- 当`mixValue = 1.0f`时，完全使用`baseColor1`
- 当`mixValue`在0-1之间时，两种颜色会线性混合

## 与顶点颜色的区别

| 特性 | 当前的baseColor方式 | 顶点颜色方式 |
|------|-------------------|-------------|
| 数据存储位置 | 材质UBO（GPU常量内存） | 顶点缓冲区（GPU顶点内存） |
| 作用范围 | 整个mesh | 每个顶点（插值到像素） |
| 内存开销 | 极小（仅几个float） | 较大（每个顶点增加12字节） |
| 修改效率 | 高（直接更新UBO） | 低（需要重新生成顶点缓冲区） |
| 适用场景 | 统一颜色或简单混合 | 复杂的逐顶点颜色渐变 |

## 总结

是的，您可以直接通过设置材质的`baseColor`为白色，使整个mesh显示为白色。这种方式比使用顶点颜色更高效、更灵活，也是当前架构推荐的做法。