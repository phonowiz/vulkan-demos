
#version 450
#extension GL_ARB_separate_shader_objects: enable


layout(location = 0) in vec4 frag_color;
//note: these two are in object space
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec3 frag_light_vec;
layout(location = 3) in vec3 frag_view_vec;

layout(location = 0) out vec4 final_color;

layout(binding = 1 ) writeonly restrict uniform image3D voxel_albedo_texture;
layout(binding = 4 ) writeonly restrict uniform image3D voxel_normal_texture;

layout(binding = 2, std140) uniform UBO
{
    mat4 inverse_view_projection;
    mat4 project_to_voxel_screen;
    vec3 voxel_coords;
} ubo;

void main()
{
    
    vec3 N = normalize(frag_normal);
    vec3 L = normalize(frag_light_vec);
    
    vec3 V = normalize(frag_view_vec);
    //TODO: reflect isn't being used right now, will come into play once we have
    //these cones ready
    //vec3 R = reflect (-L,N);

    //TODO: what should the ambient term be?
    vec3 ambient =  vec3(.08f, .08f, .08f);
    //TODO: transparncy isn't being considered here
    vec3 diffuse = max(dot(N,L), 0.0f) * frag_color.xyz;
    
    //TODO: if you ever add specular cones, this needs to be put back in
    //vec3 specular = pow(max(dot(R,V), 0.0f), 16.0f) * vec3(1.35f, 1.35f, 1.35f);
    
    
    vec4 ndc = vec4(float(gl_FragCoord.x)/ubo.voxel_coords.x, float(gl_FragCoord.y)/ubo.voxel_coords.y, gl_FragCoord.z, 1.0f);
    //scale to range[-1,1], that's the ndc range
    ndc.xy = (2.0f * ndc.xy) - 1.0f;
    
    vec4 world_coords = ubo.inverse_view_projection * ndc;
    vec4 voxel_proj = ubo.project_to_voxel_screen * world_coords;

    //normalized device coords once again...
    ndc = voxel_proj / voxel_proj.w;

    //scale to range [0,1], that is the texture range
    ndc.xy = (ndc.xy + 1) * .5f;
    ndc.xy = 1.0f - ndc.xy;
    
    int voxel_depth = int(ceil(ubo.voxel_coords.z * ndc.z));
    int voxle_height = int(ceil(ubo.voxel_coords.y * ndc.y));
    int voxel_width = int(ceil(ubo.voxel_coords.x * ndc.x));
    ivec3 voxel = ivec3(voxel_width, voxle_height, voxel_depth);
    
    imageStore(voxel_albedo_texture, voxel, vec4(diffuse.xyz, .8f));
    //TODO: We can likely shrink the size of the normal texture to a vec2 instead of a vec4
    imageStore(voxel_normal_texture, voxel, vec4(N.xyz,1.0f));
}



