
#version 450


layout (location = 0) in vec2 in_uv_coord;
layout (location = 1) in vec4 in_color;
layout (location = 2) in vec3 in_position;
layout (location = 3) in vec3 in_normal;
layout (location = 4) in mat3 tbn;

layout (location = 0) out vec4 out_albedo;
layout (location = 1) out vec4 out_normals;
layout (location = 2) out vec4 out_positions;
layout (location = 3) out vec4 out_depth;

layout (binding = 2) uniform sampler2D albedos;
layout (binding = 3) uniform sampler2D normals;
layout (binding = 4) uniform sampler2D metalness;
layout (binding = 5) uniform sampler2D roughness;
layout (binding = 6) uniform sampler2D occlusion;

//based off of: https://aras-p.info/texts/CompactNormalStorage.html and
//https://en.wikipedia.org/wiki/Lambert_azimuthal_equal-area_projection


vec2 encode (vec3 n)
{
    float f = sqrt(8.0f*n.z+8.0f);
    return n.xy / f + 0.5;
}

void main()
{
    out_albedo.xyz = texture(albedos, in_uv_coord).xyz;
    out_albedo.w = texture(occlusion, in_uv_coord).x;
    
    vec3 normal = texture(normals, in_uv_coord).xyz;
    
    normal = normal * 2.0f - 1.0f;

    normal = normalize(tbn * normal);
    
    vec2 normal_compressed = encode(normal);
    
    out_normals.xy = normal_compressed;
    out_normals.z = texture(metalness, in_uv_coord).x;
    out_normals.w = texture(roughness, in_uv_coord).x;
    
    if(out_albedo == vec4(0))
    {
        out_albedo = in_color;
    }
    
    out_positions = vec4(in_position,1.0f);
    
    out_depth.x = gl_FragCoord.z * gl_FragCoord.w;
    out_depth.yzw = vec3(.0f);
}

