#version 450

layout(location=0) in vec3 a_Pos;
layout(location=1) in vec2 a_Texcoord; // 保留但不使用
layout(location=2) in vec3 a_Normal;

layout(push_constant) uniform PushConstants {
    mat4 matrix;
    uint colorType;
} PC;

layout(location=0) out vec4 vertexColor;

void main() {

    gl_Position = PC.matrix * vec4(a_Pos, 1.f);
    

    vertexColor = vec4(a_Normal * 0.5 + 0.5, 1.0);
}