
#version 450
#extension GL_ARB_separate_shader_objects: enable

out gl_PerVertex
{
    vec4 gl_Position;
};



//these are vertex attributes in the c++ side.  In the pipeline look for "vertex input" structures
layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv_coord;
layout(location = 3) in vec3 normal;


layout(location = 0) out vec4 vertex_color;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_light_vec;
layout(location = 3) out vec3 out_view_vec;

//this is bound using the descriptor set, at binding 0 on the vertex side
layout(binding = 0, std140) uniform UBO
{
    mat4 view;
    mat4 projection;
    vec3 light_position;
    vec3 eye_position;

} ubo;

layout(binding = 3, std140) uniform DYNAMIC_UBO
{
    mat4 model;
}d_ubo;

void main()
{
    gl_Position = ubo.projection * ubo.view * d_ubo.model * vec4(pos, 1.0f);
    
    vec4 world_pos = d_ubo.model * vec4(pos,1.f);
    
    vec3 wrold_space_light_vec = ubo.light_position.xyz - world_pos.xyz;
    vec3 world_space_view_vec = ubo.eye_position.xyz - world_pos.xyz;
    
    vertex_color = color;
    out_normal = (d_ubo.model * vec4(normal,0)).xyz;
    out_light_vec = wrold_space_light_vec;
    out_view_vec = world_space_view_vec;
}
