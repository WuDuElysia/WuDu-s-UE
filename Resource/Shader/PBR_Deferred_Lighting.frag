#version 450

// 从顶点着色器输入的屏幕空间UV
layout(location=0) in vec2 v_UV;

// 输出累积直接光照
layout(location=0) out vec4 outLighting;

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

// 光源UBO结构体（与前向渲染一致）
struct LightUbo {
    vec4 position;
    vec4 directionAndRange;
    vec4 colorAndIntensity;
    float spotInnerCutoff;
    float spotOuterCutoff;
    float attenuationConstant;
    float attenuationLinear;
    float attenuationQuadratic;
    uint type;
    uint enabled;
    float padding;
};

// 光照UBO (Set 2, Binding 0)
layout(set=2, binding=0, std140) uniform LightingUbo {
    LightUbo lights[16];
    vec4 ambientColorAndIntensity;
    uint numLights;
    uint padding0;
    uint padding1;
    uint padding2;
} lightingUbo;

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
// PBR核心函数 (复用前向渲染)
// ============================================================

// GGX/Trowbridge-Reitz 法线分布函数
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0000001);
}

// Schlick-GGX 几何遮蔽（单方向）
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Smith 几何遮蔽（双方向）
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// Schlick 菲涅尔近似
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ============================================================
// 光源衰减 (复用前向渲染)
// ============================================================
float CalculateAttenuation(float distance, float range) {
    float attenuation = 1.0 / (distance * distance + 0.0001);
    if (range > 0.0) {
        float effectiveRange = range * 2.0;
        float factor = distance / effectiveRange;
        float smoothFactor = clamp(1.0 - factor * factor, 0.0, 1.0);
        attenuation *= smoothFactor * smoothFactor;
    }
    return attenuation;
}

// ============================================================
// 计算单个光源的PBR贡献 (复用前向渲染)
// ============================================================
vec3 CalculateLightContribution(LightUbo light, vec3 N, vec3 V, vec3 F0,
                                 vec3 albedo, float metallic, float roughness, vec3 worldPos) {
    if (light.enabled == 0u) return vec3(0.0);

    vec3 lightColor    = light.colorAndIntensity.rgb;
    float lightIntensity = light.colorAndIntensity.a;
    vec3 lightDir      = light.directionAndRange.xyz;
    float lightRange   = light.directionAndRange.w;

    vec3 L;
    vec3 radiance;

    if (light.type == 0u) {
        // 方向光：无衰减，方向固定
        L = normalize(-lightDir);
        radiance = lightColor * lightIntensity;
    }
    else if (light.type == 1u) {
        // 点光源：物理平方反比衰减
        vec3 lightPos = light.position.xyz;
        L = normalize(lightPos - worldPos);
        float distance = length(lightPos - worldPos);
        float attenuation = CalculateAttenuation(distance, lightRange);
        radiance = lightColor * lightIntensity * attenuation;
    }
    else if (light.type == 2u) {
        // 聚光灯：平方反比衰减 + 锥形衰减
        vec3 lightPos = light.position.xyz;
        L = normalize(lightPos - worldPos);
        float distance = length(lightPos - worldPos);
        float attenuation = CalculateAttenuation(distance, lightRange);

        float theta = dot(L, normalize(-lightDir));
        float epsilon = light.spotInnerCutoff - light.spotOuterCutoff;
        float spotEffect = clamp((theta - light.spotOuterCutoff) / max(epsilon, 0.0001), 0.0, 1.0);

        radiance = lightColor * lightIntensity * attenuation * spotEffect;
    }
    else {
        return vec3(0.0);
    }

    // Cook-Torrance BRDF
    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    // 镜面反射
    vec3  numerator   = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3  specular    = numerator / denominator;

    // 能量守恒：kS + kD = 1
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;  // 金属无漫反射

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// ============================================================
// 主函数
// ============================================================
void main() {
    // 采样深度，跳过天空/背景像素
    float depth = texture(gDepth, v_UV).r;
    if (depth >= 1.0) {
        outLighting = vec4(0.0, 0.0, 0.0, 1.0);
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

    // 累积所有光源的直接光照
    vec3 Lo = vec3(0.0);
    for (uint i = 0u; i < lightingUbo.numLights && i < 16u; i++) {
        Lo += CalculateLightContribution(lightingUbo.lights[i], N, V, F0,
                                          albedo, metallic, roughness, worldPos);
    }

    outLighting = vec4(Lo, 1.0);
}
