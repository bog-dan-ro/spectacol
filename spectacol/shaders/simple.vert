#version 450

layout(push_constant) uniform Push
{
        vec4 SourceSize;
        vec4 OriginalSize;
        vec4 OutputSize;
        uint FrameCount;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
        mat4 MVP;
} global;

layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
}
