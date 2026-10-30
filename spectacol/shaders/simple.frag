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

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
   FragColor = vec4(texture(Source, vTexCoord).rgb, 1.0);
}
