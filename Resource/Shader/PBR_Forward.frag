#version 450

// 从顶点着色器输入的数据
layout(location=1) in vec2 v_Texcoord;
layout(location=2) in vec3 v_WorldPos;
layout(location=3) in vec3 v_Normal;
layout(location=4) in vec3 v_Tangent;
layout(location=5) in vec3 v_Bitangent;

// 输出颜色
layout(location=0) out vec4 fragColor;

// 帧UBO (Set 0, Binding 0)
layout(set=0, binding=0, std140) uniform FrameUbo{
    mat4  projMat;
    mat4  viewMat;
    vec3  camPos;
    float _pad0;
    ivec2 resolution;
    uint  frameId;
    float time;
} frameUbo;

// 纹理参数结构体
struct TextureParam{
    int   enable;
    float uvRotation;
    vec4  uvTransform;
};

// PBR材质UBO (Set 1, Binding 0)
layout(set=1, binding=0, std140) uniform PBRMaterialUbo{
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float aoFactor;
    float emissiveFactor;
    TextureParam baseColorTextureParam;
    TextureParam normalTextureParam;
    TextureParam metallicRoughnessTextureParam;
    TextureParam aoTextureParam;
    TextureParam emissiveTextureParam;
} materialUbo;

// 光源UBO结构体
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
layout(set=2, binding=0, std140) uniform LightingUbo{
    LightUbo lights[16];
    vec4 ambientColorAndIntensity;
    uint numLights;
    uint padding0;
    uint padding1;
    uint padding2;
} lightingUbo;

// 纹理资源 (Set 3)
layout(set=3, binding=0) uniform sampler2D baseColorTexture;
layout(set=3, binding=1) uniform sampler2D normalTexture;
layout(set=3, binding=2) uniform sampler2D metallicRoughnessTexture;
layout(set=3, binding=3) uniform sampler2D aoTexture;
layout(set=3, binding=4) uniform sampler2D emissiveTexture;

const float PI = 3.14159265359;

// ============================================================
// UV变换
// ============================================================
vec2 getTextureUV(TextureParam param, vec2 inUV){
    vec2 retUV = inUV;
    retUV *= param.uvTransform.xy;
    float cosRot = cos(param.uvRotation);
    float sinRot = sin(param.uvRotation);
    retUV = vec2(
        retUV.x * cosRot - retUV.y * sinRot,
        retUV.x * sinRot + retUV.y * cosRot
    );
    retUV += param.uvTransform.zw;
    return retUV;
}

// ============================================================
// PBR核心函数 (参考 LearnOpenGL / Epic Games UE4 PBR)
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

// 带粗糙度的菲涅尔（用于环境光近似）
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ============================================================
// 光源衰减
// ============================================================

// 物理正确的平方反比衰减 + 平滑截断
float CalculateAttenuation(float distance, float range) {
    // 平方反比衰减
    float attenuation = 1.0 / (distance * distance + 0.0001);
    // 平滑范围截断（避免硬边）
    if (range > 0.0) {
        float factor = distance / range;
        float smoothFactor = clamp(1.0 - factor * factor, 0.0, 1.0);
        attenuation *= smoothFactor * smoothFactor;
    }
    return attenuation;
}

// ============================================================
// 计算单个光源的PBR贡献
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
// 法线贴图
// ============================================================
vec3 GetNormalFromMap() {
    if (materialUbo.normalTextureParam.enable == 0) {
        return normalize(v_Normal);
    }

    vec2 uv = getTextureUV(materialUbo.normalTextureParam, v_Texcoord);
    vec3 tangentNormal = texture(normalTexture, uv).xyz * 2.0 - 1.0;

    vec3 N = normalize(v_Normal);
    vec3 T = normalize(v_Tangent - dot(v_Tangent, N) * N);
    vec3 B = cross(N, T);
    if (dot(cross(T, B), N) < 0.0) {
        T = T * -1.0;
    }
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

// ============================================================
// ACES色调映射（比Reinhard更好的高光保留）
// ============================================================
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// ============================================================
// 主函数
// ============================================================
void main() {
    // 采样材质属性
    vec3 albedo   = materialUbo.baseColorFactor.rgb;
    float metallic  = materialUbo.metallicFactor;
    float roughness = materialUbo.roughnessFactor;
    float ao        = materialUbo.aoFactor;
    vec3 emissive   = vec3(materialUbo.emissiveFactor);

    // 纹理采样
    if (materialUbo.baseColorTextureParam.enable != 0) {
        vec2 uv = getTextureUV(materialUbo.baseColorTextureParam, v_Texcoord);
        vec3 texColor = texture(baseColorTexture, uv).rgb;
        // 基础色贴图通常在sRGB空间，转到线性空间
        albedo *= pow(texColor, vec3(2.2));
    }

    if (materialUbo.metallicRoughnessTextureParam.enable != 0) {
        vec2 uv = getTextureUV(materialUbo.metallicRoughnessTextureParam, v_Texcoord);
        vec4 mrSample = texture(metallicRoughnessTexture, uv);
        metallic  *= mrSample.b;
        roughness *= mrSample.g;
    }

    if (materialUbo.aoTextureParam.enable != 0) {
        vec2 uv = getTextureUV(materialUbo.aoTextureParam, v_Texcoord);
        ao *= texture(aoTexture, uv).r;
    }

    if (materialUbo.emissiveTextureParam.enable != 0) {
        vec2 uv = getTextureUV(materialUbo.emissiveTextureParam, v_Texcoord);
        emissive *= texture(emissiveTexture, uv).rgb;
    }

    // 确保粗糙度不为0（避免NDF除零）
    roughness = max(roughness, 0.04);

    // 法线和视角方向
    vec3 N = GetNormalFromMap();
    vec3 V = normalize(frameUbo.camPos - v_WorldPos);

    // 计算F0（基础反射率）
    // 非金属统一用0.04，金属用albedo作为F0
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // 累积所有光源的直接光照
    vec3 Lo = vec3(0.0);
    for (uint i = 0u; i < lightingUbo.numLights && i < 16u; i++) {
        Lo += CalculateLightContribution(lightingUbo.lights[i], N, V, F0,
                                          albedo, metallic, roughness, v_WorldPos);
    }

    // 环境光（简易近似，后续可替换为IBL）
    // 使用带粗糙度的菲涅尔来近似环境光的镜面/漫反射比例
    vec3 F_env = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS_env = F_env;
    vec3 kD_env = (1.0 - kS_env) * (1.0 - metallic);

    vec3 ambientColor = lightingUbo.ambientColorAndIntensity.rgb;
    float ambientIntensity = lightingUbo.ambientColorAndIntensity.a;

    // 漫反射环境光
    vec3 diffuseAmbient = kD_env * albedo * ambientColor * ambientIntensity;
    // 镜面环境光近似（没有IBL时用简单的环境色近似）
    vec3 specularAmbient = F_env * ambientColor * ambientIntensity * 0.2;

    vec3 ambient = (diffuseAmbient + specularAmbient) * ao;

    // 自发光
    vec3 result = ambient + Lo + emissive;

    // ACES色调映射
    result = ACESFilm(result);

    // Gamma校正
    result = pow(result, vec3(1.0 / 2.2));

    fragColor = vec4(result, 1.0);
}
