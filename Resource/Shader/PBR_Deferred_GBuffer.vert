#version 450
#extension GL_KHR_vulkan_glsl : enable

// 顶点属性 — 注意：与前向渲染不同，location=1 是 TexCoord，location=2 是 Normal
layout(location=0) in vec3 a_Pos;
layout(location=1) in vec2 a_Texcoord;
layout(location=2) in vec3 a_Normal;
layout(location=3) in vec3 a_Tangent;
layout(location=4) in vec3 a_Bitangent;

// 输出到片段着色器
out gl_PerVertex{
    vec4 gl_Position;
};

layout(location=0) out vec2 v_Texcoord;
layout(location=1) out vec3 v_WorldPos;
layout(location=2) out vec3 v_Normal;
layout(location=3) out vec3 v_Tangent;
layout(location=4) out vec3 v_Bitangent;

// 帧 UBO (Set 0, Binding 0) — DeferredFrameUbo
layout(set=0, binding=0, std140) uniform DeferredFrameUbo{
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

// Push Constant (模型矩阵和法线矩阵)
// 注意：std430 布局中 mat3 存储为 3 个 vec4 列（每列填充到 16 字节）
layout(push_constant) uniform PushConstants{
    mat4 modelMat;
    mat3 normalMat;
} PC;

void main(){
    // 计算世界空间位置
    vec4 worldPos = PC.modelMat * vec4(a_Pos, 1.0);
    v_WorldPos = worldPos.xyz;

    // 计算世界空间法线、切线、副切线
    v_Normal    = normalize(PC.normalMat * a_Normal);
    v_Tangent   = normalize(PC.normalMat * a_Tangent);
    v_Bitangent = normalize(PC.normalMat * a_Bitangent);

    // 传递纹理坐标
    v_Texcoord = a_Texcoord;

    // 计算裁剪空间位置
    gl_Position = frameUbo.projMat * frameUbo.viewMat * worldPos;
}
