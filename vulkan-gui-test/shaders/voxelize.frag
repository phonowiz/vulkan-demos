
#version 450
#extension GL_ARB_separate_shader_objects: enable


layout(location = 0) in vec3 frag_color;

layout(location = 0) out vec4 final_color;
layout(binding = 1 ) writeonly restrict uniform image3D voxel_texture;

layout(binding = 2) uniform UBO
{
    uint voxel_world_width;
    uint voxel_world_height;
    
} ubo;

void main()
{
    
    //TODO: VERY SIMPLE LIGHTING NEEDS TO HAPPEN HERE
//    vec3 N = normalize(fragNormal);
//    vec3 L = normalize(fragLightVec);
//    vec3 V = normalize( fragViewVec);
//    vec3 R = reflect (-L,N);
//
//    vec3 ambient = fragColor * 0.1f;
//    vec3 diffuse = max(dot(N,L), 0.0f) * fragColor;
//    vec3 specular = pow(max(dot(R,V), 0.0f), 16.0f) * vec3(1.35f, 1.35f, 1.35f);
    
    if(gl_FragCoord.x == 32 && gl_FragCoord.y == 32)
    {
         imageStore(voxel_texture, ivec3(32u,32u, 0u), vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
    else
        imageStore(voxel_texture, ivec3(gl_FragCoord.x, ubo.voxel_world_width - gl_FragCoord.y, 0u), vec4(0.0f, 1.0f, 1.0f, 1.0f));
   
    
    final_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}



