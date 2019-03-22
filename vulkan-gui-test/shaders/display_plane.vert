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


//based off of: https://stackoverflow.com/questions/41332007/opengl-2d-orthographic-rendering-for-gui-the-modern-way
void main()
{
    //vec2 viewport = vec2(dimensions.width, dimensions.height);
    //vec2 viewport = vec2(1024.0f, 768.0f);
    //todo: the division could be a constant
    //gl_Position = vec4(2 * pos.xy / viewport.xy - 1.0f, 0.0f, 1.0f);
    gl_Position = vec4(pos,1.0f);
    fragColor = vec3(0.0f, 0.0f, 0.0f);
    fragUVCoord = inUVCoord;
}

