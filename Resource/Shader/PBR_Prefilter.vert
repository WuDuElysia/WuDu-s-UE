#version 450

layout(location=0) in vec3 a_Pos;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pc;

layout(location=0) out vec3 v_LocalPos;

void main() {
    v_LocalPos = a_Pos;
    gl_Position = pc.mvp * vec4(a_Pos, 1.0);
}
