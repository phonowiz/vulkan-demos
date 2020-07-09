
#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv_coord;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;

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
layout(location = 2) out vec3 out_position;
layout(location = 3) out vec3 out_normal;
layout(location = 4) out mat3 out_tbn;



void main()
{
    gl_Position = ubo.projection * ubo.view * dynamic_b.model * vec4(pos, 1.0f);
    
    out_uv_coord = uv_coord;
    out_color = color;
    out_position = (dynamic_b.model * vec4(pos, 1.0f)).xyz;
    
    //this code is based off of:
    //https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    
    vec3 N = normalize(ubo.view * dynamic_b.model * vec4(normal, 0.0f)).xyz;
    vec3 T = normalize(ubo.view * dynamic_b.model * vec4(tangent, 0.0f)).xyz;
    
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N.xyz,T.xyz);
    out_tbn = mat3(T, B, N);
    out_tbn = transpose(inverse(out_tbn));
    out_normal = normalize(dynamic_b.model * vec4(normal,0)).xyz;

}
