#version 450

// 从顶点着色器输入的数据
layout(location=0) in vec2 v_Texcoord;
layout(location=1) in vec3 v_WorldPos;
layout(location=2) in vec3 v_Normal;
layout(location=3) in vec3 v_Tangent;
layout(location=4) in vec3 v_Bitangent;

// GBuffer 输出（3 个颜色附件）
layout(location=0) out vec4 outBaseColor;       // 基础颜色 + Alpha
layout(location=1) out vec4 outNormalEmissive;   // 世界法线 + 自发光强度
layout(location=2) out vec4 outMetallicRoughnessAO; // 金属度 + 粗糙度 + AO

// 纹理参数结构体（与前向渲染一致）
struct TextureParam{
    int   enable;
    float uvRotation;
    vec4  uvTransform;
};

// PBR 材质 UBO (Set 1, Binding 0)（与前向渲染一致）
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

// 材质纹理资源 (Set 2)
layout(set=2, binding=0) uniform sampler2D baseColorTexture;
layout(set=2, binding=1) uniform sampler2D normalTexture;
layout(set=2, binding=2) uniform sampler2D metallicRoughnessTexture;
layout(set=2, binding=3) uniform sampler2D aoTexture;
layout(set=2, binding=4) uniform sampler2D emissiveTexture;

// ============================================================
// UV 变换（与前向渲染一致）
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
// 法线贴图 TBN 转换
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
// 主函数
// ============================================================
void main() {
    // 采样基础颜色
    vec3 albedo = materialUbo.baseColorFactor.rgb;
    float alpha = materialUbo.baseColorFactor.a;

    if (materialUbo.baseColorTextureParam.enable != 0) {
        vec2 uv = getTextureUV(materialUbo.baseColorTextureParam, v_Texcoord);
        vec4 texColor = texture(baseColorTexture, uv);
        // sRGB → 线性空间转换
        albedo *= pow(texColor.rgb, vec3(2.2));
        alpha *= texColor.a;
    }

    // 采样金属度和粗糙度
    float metallic  = materialUbo.metallicFactor;
    float roughness = materialUbo.roughnessFactor;

    if (materialUbo.metallicRoughnessTextureParam.enable != 0) {
        vec2 uv = getTextureUV(materialUbo.metallicRoughnessTextureParam, v_Texcoord);
        vec4 mrSample = texture(metallicRoughnessTexture, uv);
        metallic  *= mrSample.b;  // B 通道 = 金属度
        roughness *= mrSample.g;  // G 通道 = 粗糙度
    }

    // 采样 AO
    float ao = materialUbo.aoFactor;

    if (materialUbo.aoTextureParam.enable != 0) {
        vec2 uv = getTextureUV(materialUbo.aoTextureParam, v_Texcoord);
        ao *= texture(aoTexture, uv).r;
    }

    // 采样自发光
    float emissiveIntensity = materialUbo.emissiveFactor;

    if (materialUbo.emissiveTextureParam.enable != 0) {
        vec2 uv = getTextureUV(materialUbo.emissiveTextureParam, v_Texcoord);
        // 取自发光纹理的亮度作为强度调制
        vec3 emissiveTex = texture(emissiveTexture, uv).rgb;
        emissiveIntensity *= (emissiveTex.r + emissiveTex.g + emissiveTex.b) / 3.0;
    }

    // 确保粗糙度不为 0（避免后续 NDF 除零）
    roughness = max(roughness, 0.04);

    // 获取世界空间法线（TBN 转换）
    vec3 worldNormal = GetNormalFromMap();

    // 写入 GBuffer
    outBaseColor            = vec4(albedo, alpha);
    outNormalEmissive       = vec4(worldNormal, emissiveIntensity);
    outMetallicRoughnessAO  = vec4(metallic, roughness, ao, 0.0);
}
