#version 450

layout(location=0) in vec3 v_rayDir;
layout(location=0) out vec4 outColor;

// ============================================================
// Uniform Buffer Objects
// ============================================================

layout(set=0, binding=0) uniform FrameUbo {
    mat4  projMat;
    mat4  viewMat;
    vec3  camPos;
    float _pad0;
    ivec2 resolution;
    uint  frameId;
    float time;
} frameUbo;

layout(set=1, binding=0) uniform SkyboxUbo {
    float timeScale;
    float exposure;
    float starIntensity;
    float auroraHeight;
} skyboxUbo;

// ============================================================
// Hash Functions
// ============================================================

float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

vec2 hash2(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)),
             dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453123);
}

float hash21(vec2 p) {
    float h = dot(p, vec2(127.1, 311.7));
    return fract(sin(h) * 43758.5453123);
}

// ============================================================
// Triangle Wave & Triangle Noise
// ============================================================

float tri(float x) {
    return abs(fract(x) - 0.5);
}

float triNoise2d(vec2 p, float spd) {
    float z  = 1.8;
    float z2 = 2.5;
    float rz = 0.0;
    p *= 0.5;

    for (float i = 0.0; i < 5.0; i++) {
        vec2 bp = p;
        rz += tri(p.x + tri(p.y * 1.0)) * z;
        z  *= 0.49;
        z2 *= 1.1;
        p  *= mat2(cos(0.8), sin(0.8), -sin(0.8), cos(0.8));
        p  *= 2.0;
        p.x += spd * 3.5;
    }
    return clamp(rz / 3.5, 0.0, 1.0);
}

// ============================================================
// Star Field Generation
// ============================================================

float starField(vec3 dir, float time) {
    // Project direction onto a grid on the unit sphere
    vec3 n = normalize(dir);
    // Use spherical coordinates for grid placement
    float gridScale = 80.0;
    vec2 uv = vec2(atan(n.z, n.x), asin(clamp(n.y, -1.0, 1.0)));
    uv *= gridScale;

    vec2 cell = floor(uv);
    vec2 f    = fract(uv);

    float brightness = 0.0;

    // Check neighboring cells for star placement
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 cellId   = cell + neighbor;

            // Hash-based random star position within cell
            vec2 starPos = hash2(cellId);
            vec2 diff    = neighbor + starPos - f;
            float dist   = length(diff);

            // Star visibility threshold
            float starHash = hash21(cellId);
            if (starHash > 0.85) {
                // Star size and brightness
                float starSize = 0.04 + starHash * 0.02;
                float star = 1.0 - smoothstep(0.0, starSize, dist);
                star = pow(star, 4.0);

                // Time-based twinkling
                float twinkle = sin(time * (2.0 + starHash * 4.0) + starHash * 6.2831) * 0.5 + 0.5;
                twinkle = mix(0.5, 1.0, twinkle);

                brightness += star * twinkle * starHash;
            }
        }
    }

    return brightness;
}

// ============================================================
// Aurora Rendering (Ray Marching)
// ============================================================

vec3 aurora(vec3 dir, float time) {
    // Only render above horizon
    if (dir.y <= 0.0) {
        return vec3(0.0);
    }

    vec3 color = vec3(0.0);
    float opacity = 0.0;

    // Aurora height parameters
    float baseHeight = 2.0 + skyboxUbo.auroraHeight * 5.0;
    float layerThickness = 3.0;

    // Ray march through aurora layer
    for (int i = 0; i < 50; i++) {
        // Step along the ray
        float fi = float(i) / 50.0;
        float h = baseHeight + fi * layerThickness;

        // Calculate sample point (ray-plane intersection at height h)
        float t = h / max(dir.y, 0.001);
        vec3 samplePos = dir * t;

        // Sample noise for aurora density
        vec2 noiseCoord = samplePos.xz * 0.06;
        float speed = time * 0.02;
        float density = triNoise2d(noiseCoord, speed);

        // Shape the density
        density = pow(density, 1.5) * 0.8;

        // Fade at edges of aurora layer
        float edgeFade = smoothstep(0.0, 0.2, fi) * smoothstep(1.0, 0.8, fi);
        density *= edgeFade;

        // Height-based color gradient: green (low) -> purple/pink (high)
        float heightFactor = fi;
        vec3 greenColor  = vec3(0.2, 1.0, 0.3);
        vec3 purpleColor = vec3(0.8, 0.2, 0.9);
        vec3 auroraColor = mix(greenColor, purpleColor, heightFactor);

        // Accumulate color and opacity (front-to-back compositing)
        float stepOpacity = density * (1.0 - opacity) * 0.15;
        color   += auroraColor * stepOpacity;
        opacity += stepOpacity;

        // Early exit if nearly opaque
        if (opacity > 0.95) break;
    }

    // Fade aurora near horizon for smooth transition
    float horizonFade = smoothstep(0.0, 0.15, dir.y);
    color *= horizonFade;

    return color;
}

// ============================================================
// Night Sky Background
// ============================================================

vec3 nightSky(vec3 dir) {
    // Gradient from deep blue at horizon to deep black at zenith
    vec3 horizonColor = vec3(0.05, 0.05, 0.15);
    vec3 zenithColor  = vec3(0.005, 0.005, 0.01);

    float t = clamp(dir.y, 0.0, 1.0);
    return mix(horizonColor, zenithColor, t);
}

// ============================================================
// Main Function
// ============================================================

void main() {
    vec3 rd = normalize(v_rayDir);
    float t = frameUbo.time * skyboxUbo.timeScale;

    // 1. Night sky background gradient
    vec3 color = nightSky(rd);

    // 2. Star field overlay
    color += starField(rd, t) * skyboxUbo.starIntensity;

    // 3. Aurora effect
    if (rd.y > 0.0) {
        color += aurora(rd, t);
    } else {
        // Water reflection: mirror direction and attenuate
        vec3 mirrorDir = vec3(rd.x, -rd.y, rd.z);
        vec3 reflected = aurora(mirrorDir, t);
        float attenuation = smoothstep(0.0, -0.3, rd.y) * 0.5;
        color += reflected * attenuation;
    }

    // 4. Exposure adjustment
    color *= skyboxUbo.exposure;

    outColor = vec4(color, 1.0);
}
