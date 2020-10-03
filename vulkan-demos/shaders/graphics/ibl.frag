


#version 450 core
#extension GL_ARB_separate_shader_objects : enable


//based off of
//https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf

//also based off of sascha willems example:
//https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/pbrtexture/irradiancecube.frag

layout (location = 0) out vec4 out_color;

layout(binding = 0) uniform samplerCube cubemap;
layout(binding = 1, rgba32f) writeonly restrict coherent uniform imageCube prefiltered_map;

layout(binding = 2, std140) uniform UBO
{
    float delta_phi;
    float delta_theta;
    float roughness;
    vec2  screen_size;
    mat4  positive_x;
//    mat4  negative_x;
//    mat4  positive_y;
//    //mat4  negative_y; //NO CONTRIBUTION FROM THE BOTTOM
//    mat4  positive_z;
//    mat4  negative_z;
    
}consts;


#define PI 3.1415926535897932384626433832795f

//#define TEXTURE_CUBE_MAP_POSITIVE_X    0
//#define TEXTURE_CUBE_MAP_NEGATIVE_X    1
//#define TEXTURE_CUBE_MAP_POSITIVE_Y    2
//#define TEXTURE_CUBE_MAP_NEGATIVE_Y    3
//#define TEXTURE_CUBE_MAP_POSITIVE_Z    4
//#define TEXTURE_CUBE_MAP_NEGATIVE_Z    5
//
//#define INTEGRATE_THETA(theta_iter, total_theta_iters) \
//    theta += HALF_PI/total_theta_iters * theta_iter; \
//    {                                                                           \
//        vec3 sampleVector = cos(theta) * N + sin(theta) * tempVec;              \
//        color += texture(cubemap, sampleVector).rgb * cos(theta) * sin(theta);  \
//        sampleCount++;                                                          \
//    }
//
//
//#define INTEGRATE( i, SAMPLE_COUNT ) \
//        Xi = Hammersley(i, SAMPLE_COUNT); \
//        H  = ImportanceSampleGGX(Xi, N, consts.roughness); \
//        L  = normalize(2.0 * dot(V, H) * H - V); \
//\
//        NdotL = max(dot(N, L), 0.0); \
//        if(NdotL > 0.0) \
//        { \
//            prefilteredColor += texture(cubemap, L).rgb * NdotL; \
//            totalWeight      += NdotL; \
//        }\
//


float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;
    
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
    
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    
    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

//this is the first sum approximated using the imaged based lighting integral in the Epic paper linked above
vec4 PrefilterEnvMap(float roughness, vec3 N)
{
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024;
    float totalWeight = 0.0;
    vec3 prefilteredColor = vec3(0.0);

//    vec2 Xi = vec2(0);//Hammersley(i, SAMPLE_COUNT);
//    vec3 H  = vec3(0);//ImportanceSampleGGX(Xi, N, consts.roughness);
//    vec3 L  = vec3(0);//normalize(2.0 * dot(V, H) * H - V);
//    float NdotL = 0.0f;

//    INTEGRATE(0,2)
//    INTEGRATE(1,2)
    //INTEGRATE(2,4)
    //INTEGRATE(3,4)
    
    for( int i = 0; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            prefilteredColor += texture(cubemap, L).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;
    //return vec4(1.0f);
    return vec4(prefilteredColor, 1.0f);
}

void main()
{
    vec2 uv = (2.0f * gl_FragCoord.xy / consts.screen_size.xy) -1;
    //ivec3 voxel = ivec3(gl_FragCoord.x, gl_FragCoord.y, 0);

    vec4 N = vec4(uv, 1.0f, .0f);
    //N.zw = vec2(1.0f, 0.0f);
    
    //X+
    vec3 n = normalize(N.xyz);
    
    vec4 world_normal = consts.positive_x * vec4(n, 0.0f);
    world_normal.y *= -1.0f;
    
    //TODO: you'll need to compute the reflection vector
    out_color = PrefilterEnvMap(consts.roughness, world_normal.xyz);
}
