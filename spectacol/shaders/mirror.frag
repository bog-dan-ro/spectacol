#version 450

layout(location = 0) in vec2 coord;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
     mat4 qt_Matrix;
     float qt_Opacity;
};
layout(binding = 1) uniform sampler2D source;

void main()
{
    fragColor = texture(source, vec2(coord.x, 1.0 - coord.y)) * (0.6 - coord.y) * sin(3.14 * coord.x) * qt_Opacity;
}
