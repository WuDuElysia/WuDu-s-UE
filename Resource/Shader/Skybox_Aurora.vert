#version 450

out gl_PerVertex {
    vec4 gl_Position;
};

layout(set=0, binding=0) uniform FrameUbo {
    mat4  projMat;
    mat4  viewMat;
    vec3  camPos;
    float _pad0;
    ivec2 resolution;
    uint  frameId;
    float time;
} frameUbo;

layout(location=0) out vec3 v_rayDir;

void main() {
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 pos = positions[gl_VertexIndex];

    mat4 invProj = inverse(frameUbo.projMat);
    vec4 viewDir = invProj * vec4(pos, 1.0, 1.0);
    viewDir.xyz /= viewDir.w;

    mat3 invViewRot = transpose(mat3(frameUbo.viewMat));
    v_rayDir = normalize(invViewRot * viewDir.xyz);

    gl_Position = vec4(pos, 1.0, 1.0);
}
