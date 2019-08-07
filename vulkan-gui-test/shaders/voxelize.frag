
#version 450
#extension GL_ARB_separate_shader_objects: enable


layout(location = 0) in vec3 frag_color;

layout(location = 0) out vec4 final_color;
layout(binding = 1 ) writeonly restrict uniform image3D voxel_texture;

layout(binding = 2) uniform UBO
{
    vec3 voxel_coords;
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
    vec3 texture_coordinates = vec3(float(gl_FragCoord.x)/ubo.voxel_coords.x, float(gl_FragCoord.y)/ubo.voxel_coords.y, gl_FragCoord.z);
    
    
    //we flip here because y is upside down when we render the texture, it's a vulkan thing...
    texture_coordinates.y = 1.0f - texture_coordinates.y;
    
    ivec3 voxel = ivec3(ubo.voxel_coords * texture_coordinates);

    imageStore(voxel_texture, voxel, vec4(0.0f, 1.0f, 1.0f, 1.0f));
   
    
    final_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}



