#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv_coord;
layout(location = 3) in vec3 normal;


layout(binding = 0, std140) uniform UBO
{
    mat4 view;
    mat4 projection;
    vec3 lightPosition;
} ubo;

layout(binding = 1,std140) uniform DYNAMIC
{
    mat4 model;
}dynamic_b;



layout (location = 0) out vec4 out_normal;
layout (location = 1) out vec4 out_albedo;
layout (location = 2) out vec4 out_world_pos;



void main()
{
    gl_Position = ubo.projection * ubo.view * dynamic_b.model * vec4(pos, 1.0f);
    
    vec4 pos_world = dynamic_b.model * vec4(pos, 1.0f);
    vec3 normal_view = ( ubo.view * dynamic_b.model * vec4(normal,0.0f)).xyz;
    normal_view = normalize(normal_view);
    out_normal = vec4(normal_view, 1.0f);
    out_albedo = color;
    out_world_pos = pos_world;
}
