#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 inUVCoord;
layout(location = 3) in vec3 inNormal;


layout(binding = 0) uniform UBO
{
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 lightPosition;
} ubo;




layout (location = 0) out vec4 outNormal;
layout (location = 1) out vec4 outAlbedo;
layout (location = 2) out vec4 outWorldPos;



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
    
    outNormal = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    outAlbedo = vec4(0.0f, 1.0f, 0.0f, 1.0f);
    outWorldPos = pos_world;
}
