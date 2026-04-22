#version 450

// 从顶点着色器输入的屏幕空间UV
layout(location=0) in vec2 v_UV;

// 输出IBL光照结果
layout(location=0) out vec4 outIBL;

// 帧UBO (Set 0, Binding 0) - 延迟渲染专用，含逆矩阵和相机位置
layout(set=0, binding=0, std140) uniform DeferredFrameUbo {
    mat4  projMat;
    mat4  viewMat;
    mat4  invProjMat;
    mat4  invViewMat;
    vec3  camPos;
    float _pad0;
    ivec2 resolution;
    uint  frameId;
    float time;
} frameUbo;

// GBuffer 采样器 (Set 1)
layout(set=1, binding=0) uniform sampler2D gBaseColor;       // 基础颜色+Alpha
layout(set=1, binding=1) uniform sampler2D gNormalEmissive;  // 世界法线+自发光强度
layout(set=1, binding=2) uniform sampler2D gMetallicRoughnessAO; // 金属度+粗糙度+AO
layout(set=1, binding=3) uniform sampler2D gDepth;           // 深度

// IBL 资源 (Set 2)
layout(set=2, binding=0) uniform samplerCube irradianceMap;  // 辐照度贴图
layout(set=2, binding=1) uniform samplerCube prefilterMap;   // 预滤波环境贴图
layout(set=2, binding=2) uniform sampler2D   brdfLUT;        // BRDF 查找表

const float PI = 3.14159265359;

// ============================================================
// 世界位置重建
// ============================================================
vec3 ReconstructWorldPos(vec2 uv, float depth) {
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 viewPos = frameUbo.invProjMat * clipPos;
    viewPos /= viewPos.w;
    vec4 worldPos = frameUbo.invViewMat * viewPos;
    return worldPos.xyz;
}

// ============================================================
// 带粗糙度的菲涅尔近似（用于IBL环境光）
// ============================================================
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ============================================================
// 主函数
// ============================================================
void main() {
    // 采样深度，跳过天空/背景像素
    float depth = texture(gDepth, v_UV).r;
    if (depth >= 1.0) {
        outIBL = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // 从GBuffer采样表面属性
    vec4 baseColorSample = texture(gBaseColor, v_UV);
    vec3 albedo = baseColorSample.rgb;

    vec4 normalEmissiveSample = texture(gNormalEmissive, v_UV);
    vec3 N = normalize(normalEmissiveSample.xyz);

    vec4 mraoSample = texture(gMetallicRoughnessAO, v_UV);
    float metallic  = mraoSample.r;
    float roughness = max(mraoSample.g, 0.04);
    float ao        = mraoSample.b;

    // 重建世界空间位置
    vec3 worldPos = ReconstructWorldPos(v_UV, depth);

    // 视角方向
    vec3 V = normalize(frameUbo.camPos - worldPos);

    // 计算F0（基础反射率）
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // 带粗糙度的菲涅尔
    float NdotV = max(dot(N, V), 0.0);
    vec3 F = FresnelSchlickRoughness(NdotV, F0, roughness);

    // 能量守恒
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    // 漫反射IBL：使用法线方向采样辐照度贴图
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuseIBL = kD * albedo * irradiance;

    // 镜面反射IBL：使用反射方向采样预滤波贴图 + BRDF LUT查找
    vec3 R = reflect(-V, N);
    float maxMipLevel = float(textureQueryLevels(prefilterMap) - 1);
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * maxMipLevel).rgb;
    vec2 brdf = texture(brdfLUT, vec2(NdotV, roughness)).rg;
    vec3 specularIBL = prefilteredColor * (F * brdf.x + brdf.y);

    // 合并IBL结果，乘以AO
    vec3 iblResult = (diffuseIBL + specularIBL) * ao;

    outIBL = vec4(iblResult, 1.0);
}
