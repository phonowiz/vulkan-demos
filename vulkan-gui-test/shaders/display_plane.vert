#version 450
#extension GL_ARB_separate_shader_objects: enable

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 inUVCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUVCoord;


layout(binding = 0) uniform Dimensions
{

    float width;
    float height;
} dimensions;


void main()
{
    gl_Position = vec4(pos,1.0f);
    fragColor = vec3(0.0f, 0.0f, 0.0f);
    fragUVCoord = inUVCoord;
}
