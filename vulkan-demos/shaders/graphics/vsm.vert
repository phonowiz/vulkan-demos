
#version 450
#extension GL_ARB_separate_shader_objects: enable

out gl_PerVertex
{
    vec4 gl_Position;
};

//these are vertex attributes in the c++ side.  In the pipeline look for "vertex input" structures
layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 inUVCoord;
layout(location = 3) in vec3 inNormal;

//this is bound using the descriptor set, at binding 0 on the vertex side
layout(binding = 0) uniform UBO
{
    mat4 view;
    mat4 projection;
    //vec3 lightPosition;
} ubo;

layout(binding = 1, std140) uniform DYNAMIC_UBO
{
    mat4 model;
}d_ubo;

void main()
{
    
    gl_Position = ubo.projection * ubo.view * d_ubo.model * vec4(pos, 1.0f);
}
