
#version 450
#extension GL_ARB_separate_shader_objects: enable


layout(location = 0) in vec4 frag_color;

layout(location = 0) out vec4 final_color;
layout(binding = 1 ) writeonly restrict uniform image3D voxel_texture;

layout(binding = 2) uniform UBO
{
    mat4 inverse_view_projection;
    mat4 project_to_voxel_screen;
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
    
    
    vec4 ndc = vec4(float(gl_FragCoord.x)/ubo.voxel_coords.x, float(gl_FragCoord.y)/ubo.voxel_coords.y, gl_FragCoord.z, 1.0f);
    //scale to range[-1,1], that's the ndc range
    ndc.xy = (2.0f * ndc.xy) - 1.0f;
    
    vec4 world_coords = ubo.inverse_view_projection * ndc;
    vec4 voxel_proj = ubo.project_to_voxel_screen * world_coords;

    //normalized device coords once again...
    ndc = voxel_proj / voxel_proj.w;

    //scale to range [0,1], that is the texture range
    ndc.xy = (ndc.xy + 1) * .5f;
    //flip because in vulkan ndc, y is upside down
    ndc.y = 1.0f - ndc.y;
    

    int voxel_depth = int(ceil(ubo.voxel_coords.z * ndc.z));
    int voxle_height = int(ceil(ubo.voxel_coords.y * ndc.y));
    int voxel_width = int(ceil(ubo.voxel_coords.x * ndc.x));
    ivec3 voxel = ivec3(voxel_width, voxle_height, voxel_depth);
    
    imageStore(voxel_texture, voxel, vec4(frag_color.xyz, .05f));
}



