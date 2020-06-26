
#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv_coord;
layout(location = 3) in vec3 normal;

layout(binding = 0, std140) uniform UBO
{
    mat4 view;
    mat4 projection;
} ubo;

layout(binding = 1,std140) uniform DYNAMIC
{
    mat4 model;
}dynamic_b;


layout(location = 0) out vec2 out_uv_coord;
layout(location = 1) out vec4 out_color;

void main()
{
    gl_Position = ubo.projection * ubo.view * dynamic_b.model * vec4(pos, 1.0f);
    
    out_uv_coord = uv_coord;
    out_color = color;
}
