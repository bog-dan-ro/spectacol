#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texcoord;

layout(location = 0) out vec2 v_texcoord;

layout(std140, binding = 1) uniform MVP {
    mat4 u_mvp;
};

void main() {
    v_texcoord = texcoord;
    gl_Position = u_mvp * position;
}
