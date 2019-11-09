
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
    int  state;
    vec4 sampling_rays[NUM_SAMPLING_RAYS];
    mat4 vox_view_projection;
    int  num_of_lods;
    
}rendering_state;


layout(binding = 6) uniform sampler3D voxel_normals;
layout(binding = 7) uniform sampler3D voxel_albedos;



//note: these are tied to enum class in deferred_renderer class, if these change, make sure
//make respective change accordingly

int ALBEDO = 0;
int NORMALS = 1;
int POSITIONS = 2;
int DEPTH = 3;
int FULL_RENDERING = 4;

float num_voxels_limit = 15.0f;

vec3 dimension_inverse = 1.0f/ rendering_state.voxel_size_in_world_space.xyz;
vec3 distance_limit = num_voxels_limit * rendering_state.voxel_size_in_world_space.xyz;
vec3 one_over_distance_limit = 1.0f/rendering_state.voxel_size_in_world_space.xyz;


vec4  albedo_lod_colors[NUM_MIP_MAPS];
vec4  normal_lod_colors[NUM_MIP_MAPS];


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

void collect_lod_colors( vec3 direction, vec3 world_position)
{
    //note: direction is assumed to be normalized
    
    //note: the point of starting at voxel box instead of 0 is that we want to
    //start sampling above the world position of the surface, see inside while loop below.
    vec3 j = rendering_state.voxel_size_in_world_space.xyz;
    //distance in terms of number of voxels to travel
    //float stop = length(distance_limit);
    //step is just advancing one voxel at a time
    //vec3 step = distance_limit.xyz / rendering_state.num_of_lods;
    
    uint lod = 0;
    vec3 world_pos = world_position;
    vec4 texture_space = rendering_state.vox_view_projection * vec4( world_pos, 1.f);
    
    //to NDC
    texture_space /= texture_space.w;
    //to 3D texture space, remember that z is already between [0,1] in vulkan
    texture_space.xy += 1.0f;
    texture_space.xy *= .5f;
    //also remember that in vulkan, y is flipped
    texture_space.y = 1.0f - texture_space.y;
    direction *= j;
    while(lod != rendering_state.num_of_lods)
    {
        texture_space.xyz *= direction * (lod + 1);
        //test if within voxel world clip space
        if( within_texture_bounds( texture_space ))
        {
            albedo_lod_colors[lod] = textureLod(voxel_albedos, texture_space.xyz, lod);
            normal_lod_colors[lod] = textureLod(voxel_normals, texture_space.xyz, lod);
        }
        else
            break;
        lod += 1;
        //proj +=
        //j += step;
        //lod = min(lod, rendering_state.num_of_lods);
    }
}

vec4 get_lod_color(float dist, vec4 lod_colors[NUM_MIP_MAPS], uint min_lod)
{
    
    float travel = dist * length(one_over_distance_limit.xyz);
    
    float fractional = float(rendering_state.num_of_lods) * travel;
    uint lod = uint(floor(fractional));
    
    fractional = fract(fractional);
    lod = min(lod, uint(rendering_state.num_of_lods - 1.0));
    lod = max(min_lod, lod);
    uint next = uint((lod != (rendering_state.num_of_lods-1)));
    uint lod2 = lod + next;
    
    vec4 blend_color = mix(lod_colors[lod], lod_colors[lod2], fractional);
    
    return blend_color;
}

vec4 get_lod_color(float distance, vec4 lod_colors[NUM_MIP_MAPS])
{
    return get_lod_color(distance, lod_colors, 0u);
}

void ambient_occlusion(vec3 j, vec3 step, vec3 world_pos, inout vec4 sample_color )
{
    float lambda = 10.50f;
    
    vec4 projection = rendering_state.vox_view_projection * vec4(world_pos, 1.0f);
    
    if(within_clipping_space( projection ))
    {
        float len = length(j);
        float attenuation = (1/(1 + len*lambda));
        attenuation =  10.f * pow(attenuation, 2.0f);
        vec4 from_lod = get_lod_color(len, albedo_lod_colors);
        from_lod.a = pow((1 -(1 - from_lod.a)), length(step * dimension_inverse));
        sample_color.a += (1 - sample_color.a) * from_lod.a * attenuation;
        sample_color.a = 1 - sample_color.a;
    }
    
}


vec4 voxel_cone_tracing( mat3 rotation, vec3 incoming_normal, vec3 incoming_position)
{
    //magic numbers chosen here were picked because it made the rendering look good.
    vec4 ambient = vec4(0.f);
    vec3 step = distance_limit.xyz / 6.0f;
    vec3 ambient_step = distance_limit.xyz /10.0f;
    vec3 one_over_voxel_size = vec3(1.0f)/rendering_state.voxel_size_in_world_space.xyz;
    
    vec4 sample_color = vec4(0.0f);
    
    for( uint i = 0; i < NUM_SAMPLING_RAYS; ++i)
    {
        vec3 direction = rotation * rendering_state.sampling_rays[i].xyz;
        direction = normalize(direction) ;
        
        collect_lod_colors(direction, incoming_position.xyz);
        
        vec3 j = rendering_state.voxel_size_in_world_space.xyz;
        
        float k = 1.0f;
        int l = 0;
        //we are advancing by a voxel and a half for every iteration
        vec3 m = j + j * .5f;
        
        while( l < 5)
        {
            vec3 world_sampling_position = m * direction + incoming_position.xyz;
            
            ambient_occlusion( m, ambient_step, world_sampling_position, sample_color);
            
            //TODO: more to do here...
            ++l;
            m += ambient_step;
        }
    }
    
    return vec4(sample_color.a, sample_color.a, sample_color.a, 1.0f);
}

vec4 direct_illumination(vec4 illumination, vec3 world_normal, vec3 world_position)
{
    vec3 surface_color = texture(albedo, frag_uv_coord).xyz;
    
    vec3 v = rendering_state.world_cam_position.xyz - world_position;
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
            
            float ndotl = max(dot(n, l),0.0f);
            
            final += (surface_color + spec * surface_color) * ndotl * rendering_state.light_color.xyz ;
        }
    }
    
    //final.xyz += (illumination.xyz);
    return vec4(final, 1.0f);
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

void main()
{
    if( rendering_state.state == ALBEDO )
    {
        out_color = texture(albedo, frag_uv_coord);
    }
    
    else if( rendering_state.state == NORMALS )
    {
        out_color = texture(normals, frag_uv_coord);
    }
    
    else if( rendering_state.state == POSITIONS)
    {
        out_color = texture(world_positions, frag_uv_coord);
    }
    else if( rendering_state.state == DEPTH )
    {
        out_color = texture(depth, frag_uv_coord );
        out_color.x = 1 - out_color.x;
        out_color.x *= 90.0f;
    }
    else
    {
        mat3 rotation;
        vec3 world_normal = texture(normals, frag_uv_coord).xyz;
        vec3 world_position = texture(world_positions, frag_uv_coord).xyz;
        branchless_onb(world_normal, rotation);
        
        //vec4 color = voxel_cone_tracing(rotation, world_normal, world_position);
        //out_color = color;
        
        //out_color = color;
        //final composite shot will be assembled here
        out_color = direct_illumination(vec4(0.0f), world_normal, world_position);
        
    }
}

