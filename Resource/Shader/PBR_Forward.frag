#version 450
#extension GL_KHR_vulkan_glsl : enable

// 从顶点着色器输入的数据
layout(location=1) in vec2 v_Texcoord;    // 纹理坐标
layout(location=2) in vec3 v_WorldPos;    // 世界空间位置
layout(location=3) in vec3 v_Normal;      // 世界空间法线
layout(location=4) in vec3 v_Tangent;     // 世界空间切线
layout(location=5) in vec3 v_Bitangent;   // 世界空间副切线

// 输出颜色
layout(location=0) out vec4 fragColor;

// 帧UBO (Set 0, Binding 0)
layout(set=0, binding=0, std140) uniform FrameUbo{
    mat4  projMat;        // 投影矩阵
    mat4  viewMat;        // 视图矩阵
    ivec2 resolution;     // 分辨率
    uint  frameId;        // 帧ID
    float time;           // 时间
} frameUbo;

// PBR材质UBO (Set 1, Binding 0)
layout(set=1, binding=0, std140) uniform PBRMaterialUbo{
    alignas(16) vec4 baseColorFactor;     // 基础颜色因子
    alignas(4) float metallicFactor;      // 金属度因子
    alignas(4) float roughnessFactor;     // 粗糙度因子
    alignas(4) float aoFactor;            // AO因子
    alignas(4) float emissiveFactor;      // 自发光因子
    alignas(16) TextureParam baseColorTextureParam;     // 基础颜色纹理参数
    alignas(16) TextureParam normalTextureParam;         // 法线纹理参数
    alignas(16) TextureParam metallicRoughnessTextureParam; // 金属度-粗糙度纹理参数
    alignas(16) TextureParam aoTextureParam;              // AO纹理参数
    alignas(16) TextureParam emissiveTextureParam;         // 自发光纹理参数
} materialUbo;

// 纹理参数结构体
struct TextureParam{
    bool  enable;         // 是否启用纹理
    float uvRotation;     // UV旋转角度
    vec4  uvTransform;    // UV变换 (x,y: 缩放, z,w: 平移)
};

// 光源UBO结构体
struct LightUbo {
    vec4 position;           // 光源位置 (方向光: w=0, 点光/聚光灯: w=1)
    vec3 direction;          // 光源方向 (仅方向光和聚光灯)
    float range;             // 光源范围 (仅点光和聚光灯)
    
    vec3 color;              // 光源颜色
    float intensity;         // 光源强度
    
    float spotInnerCutoff;   // 聚光灯内圆锥角（余弦值）
    float spotOuterCutoff;   // 聚光灯外圆锥角（余弦值）
    float attenuationConstant; // 衰减常数项
    float attenuationLinear;   // 衰减线性项
    
    float attenuationQuadratic; // 衰减二次项
    uint32_t type;             // 光源类型
    uint32_t enabled;          // 是否启用
    float padding;             // 对齐填充
};

// 光照UBO (Set 2, Binding 0)
layout(set=2, binding=0, std140) uniform LightingUbo{
    LightUbo lights[16];     // 支持最多16个光源
    vec3 ambientColor;       // 环境光颜色
    float ambientIntensity;  // 环境光强度
    uint32_t numLights;      // 实际光源数量
    uint32_t padding[3];     // 对齐填充
} lightingUbo;

// 纹理资源 (Set 3, Binding X)
layout(set=3, binding=0) uniform sampler2D baseColorTexture;       // 基础颜色纹理
layout(set=3, binding=1) uniform sampler2D normalTexture;           // 法线纹理
layout(set=3, binding=2) uniform sampler2D metallicRoughnessTexture; // 金属度-粗糙度纹理
layout(set=3, binding=3) uniform sampler2D aoTexture;                // AO纹理
layout(set=3, binding=4) uniform sampler2D emissiveTexture;           // 自发光纹理

// 纹理坐标变换函数
vec2 getTextureUV(TextureParam param, vec2 inUV){
    vec2 retUV = inUV;
    
    // 应用UV缩放
    retUV *= param.uvTransform.xy;
    
    // 应用UV旋转
    float cosRot = cos(param.uvRotation);
    float sinRot = sin(param.uvRotation);
    retUV = vec2(
        retUV.x * cosRot - retUV.y * sinRot,
        retUV.x * sinRot + retUV.y * cosRot
    );
    
    // 应用UV平移
    retUV += param.uvTransform.zw;
    
    return retUV;
}

// PBR核心函数

// 法线分布函数 (Trowbridge-Reitz GGX)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.141592653589793 * denom * denom;

    return nom / denom;
}

// 几何遮挡函数 (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// 几何遮挡函数 (Smith)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// 菲涅尔方程 (Schlick)
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 计算单个光源的贡献
vec3 CalculateLightContribution(LightUbo light, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, float ao, vec3 worldPos) {
    // 如果光源未启用，直接返回黑色
    if (light