
#version 450
#extension GL_ARB_separate_shader_objects: enable


//if you change this define, you must also change the equivalent variable in deferred_renderer.h
//also, make sure that you have as many rays as defined by this define.
#define NUM_SAMPLING_RAYS 5
#define NUM_MIP_MAPS 20

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_uv_coord;
layout(location = 0) out vec4 out_color;

//binding 0 is used in vertex shader
layout(binding = 1 ) uniform sampler2D normals;//albedo;
layout(binding = 2 ) uniform sampler2D albedo;
layout(binding = 3 ) uniform sampler2D world_positions;
layout(binding = 4 ) uniform sampler2D depth;

layout(binding = 5, std140) uniform _rendering_state
{
    vec4 world_cam_position;
    vec4 world_light_position;
    vec4 light_color;
    vec4 voxel_size_in_world_space;
    int  mode;
    vec4 sampling_rays[NUM_SAMPLING_RAYS];
    mat4 vox_view_projection;
    int  num_of_lods;
    vec3 eye_in_world_space;
    mat4 eye_inverse_view_matrix;
    
}rendering_state;

//zeroth levels
layout(binding = 6) uniform sampler3D voxel_normals;
layout(binding = 7) uniform sampler3D voxel_albedos;

//mipmap levels.  moltenvk doesn't support mip maps for sampler3D, only texture2d_array
layout(binding = 8) uniform sampler3D voxel_albedos1;
layout(binding = 9) uniform sampler3D voxel_albedos2;
layout(binding = 10) uniform sampler3D voxel_albedos3;
layout(binding = 11) uniform sampler3D voxel_albedos4;
layout(binding = 12) uniform sampler3D voxel_albedos5;


layout(binding = 13) uniform sampler3D voxel_normals1;
layout(binding = 14) uniform sampler3D voxel_normals2;
layout(binding = 15) uniform sampler3D voxel_normals3;
layout(binding = 16) uniform sampler3D voxel_normals4;
layout(binding = 17) uniform sampler3D voxel_normals5;



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

float voxel_jump = 1.8f;
float num_voxels_limit = 10.0f;

vec3 dimension_inverse = 1.0f/ rendering_state.voxel_size_in_world_space.xyz;
vec3 distance_limit = num_voxels_limit * rendering_state.voxel_size_in_world_space.xyz;
vec3 one_over_distance_limit = 1.0f/rendering_state.voxel_size_in_world_space.xyz;


vec4  albedo_lod_colors[NUM_MIP_MAPS];
vec4  normal_lod_colors[NUM_MIP_MAPS];

//note: moltenvk doesn't support lod's for sampler3D textures, it only supports lods for texture2d arrays
//this is the reason I have this function here
vec4 sample_lod_texture(int texture_type, vec3 coord, uint level)
{
    if( texture_type == ALBEDO )
    {
        if( level == 0)
        {
            return texture(voxel_albedos, coord);
        }
        else if( level == 1)
        {
            return texture(voxel_albedos1, coord);
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
            return texture(voxel_normals, coord);
        }
        else if( level == 1)
        {
            return texture(voxel_normals1, coord);
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
            break;
        }

        lod += 1;
        j += step * .8f;
        lod = min(lod, rendering_state.num_of_lods);
    }
}

void ambient_occlusion(vec3 world_pos, inout vec4 sample_color )
{
    float lambda = 2.0f;

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

//section 7 and 8.1 of the paper
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
    
    //TODO: tweak numbers above...
    float hack = 1.6f;
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

vec4 direct_illumination( vec3 world_normal, vec3 world_position)
{
    vec3 surface_color = texture(albedo, frag_uv_coord).xyz;
    
    vec3 v = rendering_state.eye_in_world_space.xyz - world_position;
    v = normalize(v);
    
    vec3 n = normalize(world_normal);
    
    vec3 final = vec3(0.0f);
    
    //todo: let's add support for more lights in the future
    //for(int i = 0; i < numberOfLights; ++i)
    {
        if(world_position != vec3(0.0f))
        {
            vec3 l = rendering_state.world_light_position.xyz - world_position;
            l = normalize(l);
            vec3 h = normalize( v + l);
            float ndoth = clamp( dot(n, h), 0.0f, 1.0f);
            
            float s = .65f;
            float spec = pow(ndoth, s);

            float ndotl = max(dot(n, l),0);
            final += (surface_color + spec * surface_color)* ndotl * rendering_state.light_color.xyz ;
        }
    }

    return vec4(final, 1.0f);
}

// based off of https://aras-p.info/texts/CompactNormalStorage.html
//and http://en.wikipedia.org/wiki/Lambert_azimuthal_equal-area_projection
vec3 decode (vec2 enc)
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

void main()
{
    if( rendering_state.mode == ALBEDO )
    {
        out_color = texture(albedo, frag_uv_coord);
    }
    
    else if( rendering_state.mode == NORMALS )
    {
        out_color = texture(normals, frag_uv_coord);
    }
    
    else if( rendering_state.mode == POSITIONS)
    {
        out_color = texture(world_positions, frag_uv_coord);
    }
    else if( rendering_state.mode == DEPTH )
    {
        out_color = texture(depth, frag_uv_coord );
        out_color.xyz = 1 - out_color.xxx;
        out_color.x *= 90.0f;
    }
    else
    {
        mat3 rotation;
        vec3 world_normal = texture(normals, frag_uv_coord).xyz;
        
        world_normal = decode(world_normal.xy);
        world_normal = (rendering_state.eye_inverse_view_matrix * vec4(world_normal.xyz,0.0f)).xyz;
        
        vec3 world_position = texture(world_positions, frag_uv_coord).xyz;
        branchless_onb(world_normal, rotation);
        
        out_color = vec4(0);
        if(world_position != vec3(0))
        {
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
                vec4 direct = direct_illumination( world_normal, world_position);
                out_color = direct;
                out_color.a = 1.0f;
            }
            else
            {
                vec4 direct = direct_illumination( world_normal, world_position);
                //full ambient light plus direct light
                direct.xyz += ambience.xyz;
                direct.xyz *= (1.0f - ambience.a);
                out_color.xyz = direct.xyz;
                out_color.a = 1.0f;
            }
        }
    }
}

