
#version 450
#extension GL_ARB_separate_shader_objects: enable

out gl_PerVertex
{
    vec4 gl_Position;
};



//these are vertex attributes in the c++ side.  In the pipeline look for "vertex input" structures
layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;


layout(location = 0) out vec4 vertex_color;

//this is bound using the descriptor set, at binding 0 on the vertex side
layout(binding = 0) uniform UBO
{
    mat4 model;
    mat4 view;
    mat4 projection;

} ubo;


void main()
{
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(pos, 1.0f);

    vertex_color = color;
}
