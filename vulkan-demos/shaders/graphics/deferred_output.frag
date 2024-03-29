
#version 450
#extension GL_ARB_separate_shader_objects: enable

//TODO: temporal antialiasing might be a better choice than FXAA, start here:
//https://gist.github.com/Erkaman/f24ef6bd7499be363e6c99d116d8734d

//if you change this define, you must also change the equivalent variable in deferred_renderer.h
//also, make sure that you have as many rays as defined by this define.
#define NUM_SAMPLING_RAYS 5
#define NUM_MIP_MAPS 20

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_uv_coord;

layout(location = 0) out vec4 out_color;

//binding 0 is used in vertex shader
layout(input_attachment_index = 0, binding = 1 ) uniform subpassInput normals;
layout(input_attachment_index = 1, binding = 2 ) uniform subpassInput albedo;
layout(input_attachment_index = 2, binding = 3 ) uniform subpassInput world_positions;
//the 3rd input attachment are the present textures...
layout(input_attachment_index = 4, binding = 4 ) uniform subpassInput depth;



#define DIRECTIONAL_LIGHT  0
#define POINT_LIGHT  1
#define MAX_LIGHTS 1

layout(binding = 5, std140) uniform _rendering_state
{
    vec4 world_cam_position;
    vec4 world_light_position[MAX_LIGHTS];
    vec4 light_color[MAX_LIGHTS];
    vec4 voxel_size_in_world_space;
    int  mode;
    vec4 sampling_rays[NUM_SAMPLING_RAYS];
    mat4 vox_view_projection;
    int  num_of_lods;
    vec3 eye_in_world_space;
    mat4 eye_inverse_view_matrix;
    mat4 light_cam_proj_matrix;
    int  light_types[MAX_LIGHTS];
    int  light_count;
    mat4 inverse_view_proj;
    vec2 screen_size;

}rendering_state;

//mipmap levels.  moltenvk doesn't support mip maps for sampler3D, only texture2d_array
//layout(binding = 8) uniform sampler3D voxel_albedos1;
layout(binding = 6) uniform sampler3D voxel_albedos2;
layout(binding = 7) uniform sampler3D voxel_albedos3;
layout(binding = 8) uniform sampler3D voxel_albedos4;
layout(binding = 9) uniform sampler3D voxel_albedos5;


//layout(binding = 13) uniform sampler3D voxel_normals1;
layout(binding = 10) uniform sampler3D voxel_normals2;
layout(binding = 11) uniform sampler3D voxel_normals3;
layout(binding = 12) uniform sampler3D voxel_normals4;
layout(binding = 13) uniform sampler3D voxel_normals5;

//variance shadow map
layout(binding = 14) uniform sampler2D vsm;

layout(binding = 15) uniform samplerCube    environment;
layout(binding = 16) uniform samplerCube    radiance_map;
layout(binding = 17) uniform samplerCube    spec_cubemap_high;
layout(binding = 18) uniform samplerCube    spec_cubemap_low;

layout(binding = 19) uniform sampler3D      color_lut;

//note: these are tied to enum class in deferred_renderer class, if these change, make sure
//make respective change accordingly

int ALBEDO = 0;
int NORMALS = 1;
int POSITIONS = 2;
int DEPTH = 3;
int FULL_RENDERING = 4;
int AMBIENT_OCCLUSION = 5;
int AMBIENT_LIGHT = 6;
int DIRECT_LIGHT = 7;
int VARIANCE_SHADOW_MAP = 8;

float voxel_jump = 1.8f;
float num_voxels_limit = 10.0f;

vec3 dimension_inverse = 1.0f/ rendering_state.voxel_size_in_world_space.xyz;
vec3 distance_limit = num_voxels_limit * rendering_state.voxel_size_in_world_space.xyz;
vec3 one_over_distance_limit = 1.0f/rendering_state.voxel_size_in_world_space.xyz;


vec4  albedo_lod_colors[NUM_MIP_MAPS];
vec4  normal_lod_colors[NUM_MIP_MAPS];

#define ALBEDO_SAMPLE pow(materialcolor().xyzw, vec4(1.0))

//note: moltenvk doesn't support lod's for sampler3D textures, it only supports lods for texture2d arrays
//this is the reason I have this function here
vec4 sample_lod_texture(int texture_type, vec3 coord, uint level)
{
    if( texture_type == ALBEDO )
    {
        if( level == 0)
        {
            //return texture(voxel_albedos, coord);
            return vec4(0);
        }
        else if( level == 1)
        {
            //return texture(voxel_albedos1, coord);
            return vec4(0);
        }
        else if( level == 2)
        {
            return texture(voxel_albedos2, coord);
        }
        else if( level == 3)
        {
            return texture(voxel_albedos3, coord);
        }
        else if( level == 4)
        {
            return texture(voxel_albedos4, coord);
        }
        else
        {
            return texture(voxel_albedos5, coord);
        }
    }
    else // texture_type == NORMALS
    {
        if( level == 0)
        {
            //return texture(voxel_normals, coord);
            return vec4(0);
        }
        else if( level == 1)
        {
            //return texture(voxel_normals1, coord);
            return vec4(0);
        }
        else if( level == 2)
        {
            return texture(voxel_normals2, coord);
        }
        else if( level == 3)
        {
            return texture(voxel_normals3, coord);
        }
        else if( level == 4)
        {
            return texture(voxel_normals4, coord);
        }
        else
        {
            return texture(voxel_normals5, coord);
        }
    }
    
    return vec4(0.0f);
}
bool within_clipping_space( vec4 pos)
{
    return
    (-pos.w <= pos.x && pos.x <= pos.w) &&
    (-pos.w <= pos.y && pos.y <= pos.w) &&
    (0 <= pos.z && pos.z <= pos.w);
}
bool within_texture_bounds( vec4 pos)
{
    return  (0.0f <= pos.x && pos.x <= 1.0f) &&
    (0.0f <= pos.y && pos.y <= 1.0f) &&
    (0.0f <= pos.z && pos.z <= 1.0f);
}

void branchless_onb(vec3 n, out mat3 rotation)
{
    //based off of "Building Orthonormal Basis, Revisited", Pixar Animation Studios
    //https://graphics.pixar.com/library/OrthonormalB/paper.pdf
    float s = int(n.z >= 0) - int(n.z < 0);
    float a = -1.0f / (s + n.z);
    float b = n.x * n.y * a;
    vec3 b1 = vec3(1.0f + s * n.x * n.x * a, s * b, -s * n.x);
    vec3 b2 = vec3(b, s + n.y * n.y * a, -n.y);
    
    rotation[0] = b1;
    rotation[1] = n;
    rotation[2] = b2;
}


void collect_lod_colors( vec3 direction, vec3 world_position)
{
    //note: direction is assumed to be normalized
    
    //note: the point of starting at voxel box instead of 0 is that we want to
    //start sampling above the world position of the surface, see inside while loop below.
    vec3 j = rendering_state.voxel_size_in_world_space.xyz;
    uint lod = 2;
    vec3 step = j*voxel_jump;
    j += step;

    while( lod != rendering_state.num_of_lods)
    {
        vec3 world_pos = world_position + j * direction;
        vec4 texture_space = rendering_state.vox_view_projection * vec4( world_pos, 1.f);
        //texture_space.xyz *= direction * (lod + 1);
        
        //test if within voxel world clip space
        if( within_clipping_space( texture_space ))
        {
            //to NDC
            texture_space /= texture_space.w;
            //to 3D texture space, remember that z is already between [0,1] in vulkan
            texture_space.xy += 1.0f;
            texture_space.xy *= .5f;
            //also remember that in vulkan, y is flipped
            texture_space.xy = 1.0f - texture_space.xy;
            
            albedo_lod_colors[lod] = sample_lod_texture(ALBEDO, texture_space.xyz, lod);
            normal_lod_colors[lod] = sample_lod_texture(NORMALS, texture_space.xyz, lod);
        }
        else
        {
            albedo_lod_colors[lod] = vec4(0);
            normal_lod_colors[lod] = vec4(0);
        }

        lod += 1;
        j += step * .8f;
        lod = min(lod, rendering_state.num_of_lods);
    }
}

void ambient_occlusion(vec3 world_pos, inout vec4 sample_color )
{
    float lambda = 4.0f;

    vec4 projection = rendering_state.vox_view_projection * vec4(world_pos, 1.0f);
    vec3 j = rendering_state.voxel_size_in_world_space.xyz;
    uint lod = 2;
    vec3 step = j*voxel_jump;
    j += step;
    
    while(lod != rendering_state.num_of_lods)
    {
        float len = length(j);
        float attenuation = (1/(1 + len*lambda));
        attenuation = pow(attenuation, 2.0f);
        
        sample_color.a += albedo_lod_colors[lod].a * attenuation;
        j += step;
        lod++;
    }
}

float toksvig_factor(vec3 normal, float s)
{
    
    //based off of "Mipmapping Normal Maps", Nvidia
    //https://developer.download.nvidia.com/whitepapers/2006/Mipmapping_Normal_Maps.pdf
    //example implementation here: http://www.selfshadow.com/sandbox/gloss.html
    
    float rlen = 1.0f/clamp(length(normal), 0.0f, 1.0f);
    //sigma = |N|/(|N| + s(1 - |N|)).  Below, we just devided numerator and denominator by |N|
    return 1.0f/(1.0f + s * (rlen - 1.0f));
}


float gaussian_lobe_distribution(vec3 normal, float variance)
{
    //based off of: https://math.stackexchange.com/questions/434629/3-d-generalization-of-the-gaussian-point-spread-function
    float sigma = variance;
    float e = 2.71828f;
    float pi = 3.14159f;
    float N = pow(2.0f, 3.0f) * pow(sigma, 2.0 * 3.0f) * pow(pi, 3.0f);
    N = 1.0f/sqrt(N);
    float power = -(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z)/(sigma * sigma);
    float gauss = N * pow(e, power);
    
    return gauss;
}


float get_variance(float distance)
{
//    vec3 travel = distance * one_over_distance_limit;
//
//    vec3 fractional = float(NUM_MIP_MAPS) * travel;
//    uint lod = uint(floor(length(fractional)));
//
//    fractional = fract(fractional);
//    lod = min(lod, NUM_MIP_MAPS);
//
//    uint next = uint((lod == (NUM_MIP_MAPS-1)));
//    uint lod2 = lod + next;
//
//    float variance = mix(coneVariances[lod], coneVariances[lod2], fractional);
    float variance = distance == 0 ? 0 : (1 - distance)/distance;
    return variance;
}

const float PI = 3.14159265359;

vec4 materialcolor()
{
    vec4 color = subpassLoad(albedo) ;
    return vec4(color.r, color.g, color.b, color.a);
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return (alpha2)/(PI * denom*denom);
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float GL = dotNL / (dotNL * (1.0 - k) + k);
    float GV = dotNV / (dotNV * (1.0 - k) + k);
    return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return  F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 BRDF(vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness, vec3 lightColor)
{
    // Precalculate vectors and dot products
    vec3 H = normalize (V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotLH = clamp(dot(L, H), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);

    // Light color fixed
    //vec3 lightColor = rendering_state.light_color.xyz;

    vec3 color = vec3(0.0);

    if (dotNL > 0.0)
    {
        // D = Normal distribution (Distribution of the microfacets)
        float D = D_GGX(dotNH, roughness);
        // G = Geometric shadowing term (Microfacets shadowing)
        float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
        // F = Fresnel factor (Reflectance depending on angle of incidence)
        vec3 F = F_Schlick(dotNV, F0);

        vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001f);
        vec3 kD = (vec3(1.0f) - F) * (1.0f - metallic);
        vec4 c = ALBEDO_SAMPLE;
        //the w component is the artist's added ambient occlusion
        color += (kD * c.xyz / PI + spec) * dotNL * lightColor * c.w;
    }

    return color;
}


//section 7 and 8.1 of orignal voxel cone tracing paper...
vec4 indirect_illumination( vec3 world_normal, vec3 world_pos, vec3 direction)
{
    vec3 j = rendering_state.voxel_size_in_world_space.xyz;
    uint lod = 3;
    vec3 step = j * voxel_jump;
    j += step ;
    
    vec4 final_color = vec4(0);
    while(lod != rendering_state.num_of_lods)
    {
        vec4 avg_normal = normal_lod_colors[lod];
        vec4 avg_albedo = albedo_lod_colors[lod];
        
        float sqrd = avg_normal.x * avg_normal.x + avg_normal.y * avg_normal.y + avg_normal.z * avg_normal.z;
        vec3 sampling_pos = world_pos + j * direction;
        
        //this is to avoid division by zero
        if( sqrd > 0.0f)
        {
            
            vec3 light_dir_in_world_space = sampling_pos.xyz - world_pos.xyz;
            
            vec3 view = normalize(rendering_state.eye_in_world_space - world_pos.xyz);

            vec3 up = world_normal;
            
            float variance = get_variance(length(avg_normal.xyz));
            
            //TODO: the paper does have instructions to use gaussian lobe distribution, but this wasn't giving me
            //visually pleasing results, commented out for this reason, but will leave here for reference.  I think
            //am doing something wrong somewhere...
            
            //float gauss = gaussian_lobe_distribution( view, variance) ;
            float gauss = 1.0f;
            
            //Blinn Phong
            vec3 light_direction = normalize(light_dir_in_world_space);
            
            vec3 h = normalize(view + light_direction);
            float ndoth = clamp(dot(up, h), 0.0f, 1.0f);
            //todo: we need a specular map where this power comes from, for now it is constant
            float s = .005f;
            float gloss = toksvig_factor(avg_normal.xyz, gauss);
            
            float p = s * gloss;
            float spec = pow(ndoth, p);
            
            float ndotl = clamp(dot(up, light_direction), 0.0f, 1.0f);
            final_color += (avg_albedo + avg_albedo * spec) * ndotl * spec;  //(avg_albedo + avg_albedo * spec) * ndotl * gloss;
        }
        
        ++lod;
        j += step;
    }
    
    //TODO: this hack should go away when we start using HDR values and gamma correction
    float hack = 1.9f;
    return vec4(final_color.xyz, 1.0f) * hack ;
}

vec4 voxel_cone_tracing( mat3 rotation, vec3 incoming_normal, vec3 incoming_position)
{
    vec3 ambient_step = distance_limit / rendering_state.num_of_lods;
    vec3 one_over_voxel_size = vec3(1.0f)/rendering_state.voxel_size_in_world_space.xyz;
    
    vec4 sample_color = vec4(0.0f);
    for( uint i = 0; i < NUM_SAMPLING_RAYS; ++i)
    {
        vec3 direction = rotation * rendering_state.sampling_rays[i].xyz;
        direction = normalize(direction) ;
        
        collect_lod_colors(direction, incoming_position.xyz);
        
        ambient_occlusion(incoming_position, sample_color);
        
        vec4 ambient_color = indirect_illumination(incoming_normal, incoming_position, direction);
        sample_color.xyz += ambient_color.xyz ;
    }
    
    return sample_color;
}

//https://knarkowicz.wordpress.com/2014/12/27/analytical-dfg-term-for-ibl/
//this function estimates the brdf 2D lut created when performing imaged based lighting
vec3 EnvDFGPolynomial( vec3 specularColor, float gloss, float ndotv )
{
    float x = gloss;
    float y = ndotv;
 
    float b1 = -0.1688;
    float b2 = 1.895;
    float b3 = 0.9903;
    float b4 = -4.853;
    float b5 = 8.404;
    float b6 = -5.069;
    float bias = clamp( min( b1 * x + b2 * x * x, b3 + b4 * y + b5 * y * y + b6 * y * y * y ), 0.0f, 1.0f );
 
    float d0 = 0.6045;
    float d1 = 1.699;
    float d2 = -0.5228;
    float d3 = -3.603;
    float d4 = 1.404;
    float d5 = 0.1939;
    float d6 = 2.661;
    float delta = clamp( d0 + d1 * x + d2 * y + d3 * x * x + d4 * x * y + d5 * y * y + d6 * x * x * x , 0.0f, 1.0f);
    float scale = delta - bias;
 
    bias *= clamp( 50.0 * specularColor.y, 0, 1 );
    return specularColor * scale + bias;
}

vec4 direct_illumination( vec3 world_normal, vec3 world_position, float metalness, float roughness)
{
    vec3 v = rendering_state.eye_in_world_space.xyz - world_position;
    v = normalize(v);
    
    vec3 n = normalize(world_normal);
    
    vec3 final = vec3(0.0f);
    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, ALBEDO_SAMPLE.xyz, metalness);
    for(int i = 0; i < rendering_state.light_count; ++i)
    {
        if(world_position != vec3(0.0f))
        {
            //for directional lights, the position is the light direction
            vec3 l = rendering_state.world_light_position[i].xyz;

            if(rendering_state.light_types[i] == POINT_LIGHT)
                l = rendering_state.world_light_position[i].xyz - world_position;

            final += BRDF( l, v, world_normal, F0, metalness, roughness, rendering_state.light_color[i].xyz );
        }
    }
    
    //ambient term based off of Willem's example: https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/pbribl/pbribl.frag
    vec3 F = F_SchlickR(max(dot(world_normal, v), 0.0), F0, roughness);
    vec3 kD = 1.0f - F;
    final += texture(radiance_map, world_normal).rgb * ALBEDO_SAMPLE.xyz * kD ;

    //IBL
    vec3 r = reflect(-v, world_normal);
    vec3 rough_reflect = texture(spec_cubemap_high,r).xyz;
    vec3 smooth_reflect = texture(spec_cubemap_low, r).xyz;
    
    vec3 ibl_reflect = mix(smooth_reflect, rough_reflect, roughness);
    //vec2 envBRDF = texture(brdfLUT, vec2(max(dot(n, v), 0.0), roughness)).rg;
    //vec3 specular = ibl_reflect * (F0 * envBRDF.x + envBRDF.y);
    vec3 specular = ibl_reflect * EnvDFGPolynomial(F0, pow(1-roughness, 4), max(dot(n, v), 0.0));
    
    return vec4(final + specular, 1.0f);
}

// based off of https://aras-p.info/texts/CompactNormalStorage.html
//and http://en.wikipedia.org/wiki/Lambert_azimuthal_equal-area_projection
vec3 decode(vec2 enc)
{
    //sphere map transform.  The idea here is that the normal is mapped to a 2D plane and then
    //transformed back to sphere.  Please see links
    vec2 fenc = enc*4.f-2.f;
    float f = dot(fenc,fenc);
    float g = sqrt(1-f/4);
    vec3 n;
    n.xy = fenc*g;
    n.z = 1-f/2;
    return n;
}

////variance shadow maps, based off of
////http://developer.download.nvidia.com/SDK/10/direct3d/Source/VarianceShadowMapping/Doc/VarianceShadowMapping.pdf
////and
////http://www.punkuser.net/vsm/vsm_paper.pdf
//
float linstep(float low, float high, float v)
{
    return clamp((v - low)/(high-low), 0.0f, 1.0f);
}

vec2 vsm_filter( vec3 moments, float fragDepth )
{
    vec2 lit = vec2(0.0f);
    float E_x2 = moments.y;
    float Ex_2 = moments.x * moments.x;
    float variance = max(E_x2 - Ex_2, 0.00002f);
    float mD = fragDepth - moments.x ;
    
    float mD_2 = mD * mD;
    float p = linstep(0.97f, 1.0f, variance / (variance + mD_2));
 
    float result = fragDepth <= moments.x  ? 1 : 0;
    lit.x = min(max( p , result ), 1.0f);
    //lit.x = result;
    return lit ; //lit.x == VSM calculation
}

float shadow_factor(vec3 world_position)
{
    vec4 texture_space = rendering_state.light_cam_proj_matrix * vec4(world_position, 1.0f);
    
    //to NDC
    texture_space /= texture_space.w;
    //to  2D texture space, remember that z is already between [0,1] in vulkan
    texture_space.xy += 1.0f;
    texture_space.xy *= .5f;
        
    vec3 moments = texture(vsm, texture_space.xy).xyz;

    //shadows look weird when they are pitch black, this doesn't happen
    //often in the real world, this hack is to improve this situation.

    return max(vsm_filter(moments, texture_space.z).x, .15f);
    
}

vec3 get_camera_vector() {

    vec2 uv = (2.0f * gl_FragCoord.xy / rendering_state.screen_size.xy) -1.0f;
    vec4 proj = vec4(uv,1.0f,1.0f);
    
    vec4 ans = rendering_state.inverse_view_proj * proj;
    return normalize(vec3(ans.x, ans.y, ans.z));
}


///////////////////////////////////////////////////
void main()
{
    //note: in a real scenario, you don't want all these branches around, this is purely for
    //demo purposes.
        
    if( rendering_state.mode == ALBEDO )
    {
        out_color = subpassLoad(albedo);
        //out_color.xyz = vec3(out_color.w);
        out_color.w =1.0f;
    }

    else if( rendering_state.mode == NORMALS )
    {
        out_color.xy = subpassLoad(normals).xy;
        
        out_color.xyz = decode(out_color.xy);
        //out_color.xyz = vec3(subpassLoad(normals).z);
        //out_color.z = 0.0f;
        out_color.w = 1.0f;
    }

    else if( rendering_state.mode == POSITIONS)
    {
        out_color = subpassLoad(world_positions);
    }
    else if( rendering_state.mode == DEPTH )
    {
        out_color = subpassLoad(depth);
        out_color.xyz = 1 - out_color.xxx;
        out_color.x *= 90.0f;
    }
    else if( rendering_state.mode == VARIANCE_SHADOW_MAP)
    {
        vec3 world_position = subpassLoad(world_positions).xyz;
        out_color = vec4(0);
        if(world_position != vec3(0))
        {
            float f = shadow_factor(world_position);
            out_color = vec4(f, f, f, 1.0f);
        }
    }
    else
    {
        vec4 dpth = subpassLoad(depth);
        
        if(dpth.r != 1.0f)
        {
            mat3 rotation;
            vec4 normal_sample = subpassLoad(normals);
            
            vec3 world_normal = decode(normal_sample.xy);
            float metalness = normal_sample.z;
            float roughness = normal_sample.w ;
            vec4 depth = subpassLoad(world_positions);
            
            world_normal = (rendering_state.eye_inverse_view_matrix * vec4(world_normal.xyz,0.0f)).xyz;

            vec3 world_position = subpassLoad(world_positions).xyz;
            branchless_onb(world_normal, rotation);


            vec4 ambience = voxel_cone_tracing(rotation, world_normal, world_position);

            if( rendering_state.mode == AMBIENT_OCCLUSION)
            {
                out_color.xyz = (1.0f - ambience.aaa);
                out_color.a = 1.0f;
            }

            else if( rendering_state.mode == AMBIENT_LIGHT)
            {
                out_color.xyz = ambience.xyz;
                out_color.a = 1.0f;
            }
            else if ( rendering_state.mode == DIRECT_LIGHT)
            {
                vec4 direct = direct_illumination( world_normal, world_position, metalness, roughness);
                out_color = direct;
                
                out_color.xyz = pow(out_color.xyz, vec3(0.4545));
                
                out_color.a = 1.0f;
            }
            else
            {
                vec4 direct = direct_illumination( world_normal, world_position, metalness, roughness);
                //full ambient light plus direct light
                float shadow = shadow_factor(world_position);
                direct.xyz *= shadow;

                direct.xyz *= ambience.xyz;
                direct.xyz *= (1.0f - ambience.a);

                out_color.xyz = direct.xyz;
                out_color.w = out_color.x * 0.2126f +  out_color.y * 0.7152f + out_color.z * 0.0722f;
                out_color.xyz = texture(color_lut, out_color.xyz).xyz;
            }
        }
        else
        {
            vec3 ray = get_camera_vector();
            
            out_color = texture(environment, ray);
            out_color.xyz = texture(color_lut, out_color.xyz).xyz;
            out_color.w = 1.0f;
        }
    }
}

