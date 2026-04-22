#version 450

// 从顶点着色器输入的屏幕空间UV
layout(location=0) in vec2 v_UV;

// 输出合并后的HDR光照结果
layout(location=0) out vec4 outMerged;

// GBuffer 采样器 (Set 0)
layout(set=0, binding=0) uniform sampler2D gBaseColor;       // 基础颜色+Alpha
layout(set=0, binding=1) uniform sampler2D gNormalEmissive;  // 世界法线+自发光强度
layout(set=0, binding=2) uniform sampler2D gMetallicRoughnessAO; // 金属度+粗糙度+AO (未使用，但布局需匹配)
layout(set=0, binding=3) uniform sampler2D gDepth;           // 深度 (未使用，但布局需匹配)

// 光照结果采样器 (Set 1)
layout(set=1, binding=0) uniform sampler2D directLightingTex; // 直接光照累积
layout(set=1, binding=1) uniform sampler2D iblTex;            // IBL累积

void main() {
    // 采样直接光照和IBL光照结果
    vec3 directLighting = texture(directLightingTex, v_UV).rgb;
    vec3 iblLighting    = texture(iblTex, v_UV).rgb;

    // 从GBuffer 1的alpha通道读取自发光强度
    float emissiveIntensity = texture(gNormalEmissive, v_UV).a;

    // 自发光贡献（简化方案：白色自发光，强度由GBuffer 1 alpha控制）
    vec3 emissive = vec3(emissiveIntensity);

    // 合并最终HDR结果
    vec3 finalHDR = directLighting + iblLighting + emissive;

    outMerged = vec4(finalHDR, 1.0);
}
