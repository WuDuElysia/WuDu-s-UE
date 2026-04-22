#version 450

out gl_PerVertex {
    vec4 gl_Position;
};

layout(set=0, binding=0) uniform FrameUbo {
    mat4 projMat;
    mat4 viewMat;
    vec3 camPos;
    float _pad0;
    ivec2 resolution;
    uint frameId;
    float time;
} frameUbo;

layout(location=0) out vec3 v_nearPos;
layout(location=1) out vec3 v_farPos;

void main() {
    // Hardcoded fullscreen triangle vertices
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 pos = positions[gl_VertexIndex];
    gl_Position = vec4(pos, 0.0, 1.0);

    // Unproject to world space
    mat4 invProjView = inverse(frameUbo.projMat * frameUbo.viewMat);
    vec4 nearH = invProjView * vec4(pos, 0.0, 1.0);  // NDC z=0 -> near plane
    vec4 farH  = invProjView * vec4(pos, 1.0, 1.0);  // NDC z=1 -> far plane
    v_nearPos = nearH.xyz / nearH.w;
    v_farPos  = farH.xyz  / farH.w;
}
