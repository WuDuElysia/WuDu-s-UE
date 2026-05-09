#version 450

layout(location=0) in vec2 v_UV;

layout(location=0) out vec4 fragColor;

// Set 0 Binding 0: DeferredFrameUbo (layout must match, not used in initial version)
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

// Set 1 Binding 0-1: Lighting samplers (layout must match, not used directly)
layout(set=1, binding=0) uniform sampler2D lightingSampler0;
layout(set=1, binding=1) uniform sampler2D lightingSampler1;

// Set 2 Binding 0: Merged HDR lighting result
layout(set=2, binding=0) uniform sampler2D mergedLightingTexture;

// ACES tone mapping (same as forward rendering ACESFilm)
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdrColor = texture(mergedLightingTexture, v_UV).rgb;

    // ACES tone mapping
    vec3 mapped = ACESFilm(hdrColor);

    // Gamma correction
    mapped = pow(mapped, vec3(1.0 / 2.2));

    fragColor = vec4(mapped, 1.0);
}
