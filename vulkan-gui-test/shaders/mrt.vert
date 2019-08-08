#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv_coord;
layout(location = 3) in vec3 normal;


layout(binding = 0) uniform UBO
{
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 lightPosition;
} ubo;




layout (location = 0) out vec4 out_normal;
layout (location = 1) out vec4 out_albedo;
layout (location = 2) out vec4 out_world_pos;



void main()
{
//    vec4 tmpPos = inPos + ubo.instancePos[gl_InstanceIndex];
//
//    gl_Position = ubo.projection * ubo.view * ubo.model * tmpPos;
//
//    outUV = inUV;
//    outUV.t = 1.0 - outUV.t;
//
//    // Vertex position in world space
//    outWorldPos = vec3(ubo.model * tmpPos);
//    // GL to Vulkan coord space
//    outWorldPos.y = -outWorldPos.y;
//
//    // Normal in world space
//    mat3 mNormal = transpose(inverse(mat3(ubo.model)));
//    outNormal = mNormal * normalize(inNormal);
//    outTangent = mNormal * normalize(inTangent);
//
//    // Currently just vertex color
//    outColor = inColor;
    
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(pos, 1.0f);
    
    vec4 pos_world = ubo.model * vec4(pos, 1.0f);
    vec3 normal_world = transpose(inverse(mat3(ubo.model))) * normalize(normal);
    out_normal = vec4(normal_world, 1.0f);
    out_albedo = color;
    out_world_pos = pos_world;
}
