
#version 450 core
#extension GL_ARB_separate_shader_objects : enable


//based off of
//https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf

//also based off of sascha willems example:
//https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/pbrtexture/irradiancecube.frag

layout (location = 0) out vec4 out_color;

layout(binding = 0) uniform samplerCube cubemap;
//layout(binding = 1, rgba32f) writeonly restrict coherent uniform imageCube radiance_map;

layout(binding = 2, std140) uniform UBO
{
    float delta_phi;
    float delta_theta;
    vec2  screen_size;
    
    mat4  positive_x;
//    mat4  negative_x;
//    mat4  positive_y;
//    //mat4  negative_y; //NO CONTRIBUTION FROM THE BOTTOM
//    mat4  positive_z;
//    mat4  negative_z;
    
}consts;


#define PI 3.1415926535897932384626433832795f

#define TEXTURE_CUBE_MAP_POSITIVE_X    0
#define TEXTURE_CUBE_MAP_NEGATIVE_X    1
#define TEXTURE_CUBE_MAP_POSITIVE_Y    2
#define TEXTURE_CUBE_MAP_NEGATIVE_Y    3
#define TEXTURE_CUBE_MAP_POSITIVE_Z    4
#define TEXTURE_CUBE_MAP_NEGATIVE_Z    5

#define INTEGRATE_THETA(theta_iter, total_theta_iters) \
    theta += HALF_PI/total_theta_iters * theta_iter; \
    {                                                                           \
        vec3 sampleVector = cos(theta) * N + sin(theta) * tempVec;              \
        color += texture(cubemap, sampleVector).rgb * cos(theta) * sin(theta);  \
        sampleCount++;                                                          \
    }


#define INTEGRATE( iter, total_iters ) \
        phi += TWO_PI/total_iters * iter; \
        theta = 0.0f;    \
        tempVec = cos(phi) * right + sin(phi) * up; \
        INTEGRATE_THETA(0, 3)           \
        INTEGRATE_THETA(1, 3)           \
        INTEGRATE_THETA(2, 3)           \


vec4 integrate(vec3 N)
{
    N = normalize(N);
    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    //return vec4(N,1.0f);
    const float TWO_PI = PI * 2.0f;
    const float HALF_PI = PI * 0.5f;

    vec3 color = vec3(0.0f);
    
    uint sampleCount = 0u;
    float phi = 0.0f;
    vec3  tempVec = vec3(0);
    float theta = 0.0f;
    
    
    //note: this is the "for" loop I try to emulate using the #define INTEGRATE.
    //For some odd reason if you try too many iterations, the for loops fail and don't return the right color (you get black).
    //I figured that if I unrolled the loop I could get more out of it, that turned out to be true up to a point.
    //there seems to be a limit on how many times you can use the texture sampling function.  I thought it was
    //a barrier issue, but those are accounted for (look in node.h" file). It is unclear to me why this limit is happening.
    //This might be as a result of the weak 2014 mac book pro am using.
    
    for (float phi = 0.0f; phi < TWO_PI; phi += consts.delta_phi)
    {

        vec3 tempVec = cos(phi) * right + sin(phi) * up;
        //theta goes around from down to up 90 degrees
        for (float theta = 0.0f; theta < HALF_PI; theta += consts.delta_theta)
        {
            vec3 sampleVector = cos(theta) * N + sin(theta) * tempVec;
            //color += sampleVector;//texture(cubemap, sampleVector).rgb;// * cos(theta) * sin(theta) * 100.0f;
            color += texture(cubemap, sampleVector).rgb * cos(theta) * sin(theta);
            sampleCount++;
        }
    }
    
//    //note: few iterations here works fine because the cube map is a low frequency texture (not too many harsh color transitions in it).
//    INTEGRATE(0,3)
//    INTEGRATE(1,3)
//    INTEGRATE(2,3)

    return vec4(PI * color / float(sampleCount), 1.0);
}

void main()
{
    vec2 uv = (2.0f * gl_FragCoord.xy / consts.screen_size.xy) -1;
    ivec3 voxel = ivec3(gl_FragCoord.x, gl_FragCoord.y, 0);
    
    vec4 N = vec4(uv, 1.0f, .0f);
    //N.zw = vec2(1.0f, 0.0f);
    //NEW
    vec3 n = normalize(N.xyz);
    vec4 world_normal = consts.positive_x * vec4(n, 0.0f);
    world_normal.y *= -1.0f;
    
    out_color = integrate(world_normal.xyz);
    //
    
//    //X+
//    vec3 n = normalize(N.xyz);
//    vec4 world_normal = consts.positive_x * vec4(n, 0.0f);
//    world_normal.y *= -1.0f;
//
//    out_color = integrate(world_normal.xyz);
//    voxel.z = TEXTURE_CUBE_MAP_POSITIVE_X;
//    imageStore(radiance_map, voxel, out_color);
//
//    //X-
//    world_normal = consts.negative_x * vec4(n, 0.0f);
//    world_normal.y *= -1.0f;
//
//    out_color = integrate(world_normal.xyz);
//    voxel.z = TEXTURE_CUBE_MAP_NEGATIVE_X;
//    imageStore(radiance_map, voxel, out_color);
//
//    //Y+
//    world_normal = consts.positive_y * vec4(n,0.0f);
//    world_normal.y *= -1.0f;
//
//    out_color = integrate(world_normal.xyz);
//    voxel.z = TEXTURE_CUBE_MAP_POSITIVE_Y;
//    imageStore(radiance_map, voxel, out_color);
//
////    NO CONTRIBUTION FROM THE BOTTOM, LIKELY THERE IS A FLOOR THERE
////    //Y-
////    world_normal = consts.negative_y * n;
////    world_normal.y *= -1.0f;
////
////    out_color = integrate(world_normal);
////    voxel = ivec3( uv, TEXTURE_CUBE_MAP_NEGATIVE_Y);
////    imageStore(radiance_map, voxel, out_color);
//
//    //Z+
//    world_normal = consts.positive_z * vec4(n, 1.0f);
//    world_normal.y *= -1.0f;
//
//    out_color = integrate(world_normal.xyz);
//    voxel.z = TEXTURE_CUBE_MAP_POSITIVE_Z;
//    imageStore(radiance_map, voxel, out_color);
//
//    //Z-
//    world_normal = consts.negative_z * vec4(n, 1.0f);
//    world_normal.y *= -1.0f;
//
//    out_color = integrate(world_normal.xyz);
//    voxel.z = TEXTURE_CUBE_MAP_NEGATIVE_Z;
//    imageStore(radiance_map, voxel, out_color);
//
//    out_color = vec4(0);
}
