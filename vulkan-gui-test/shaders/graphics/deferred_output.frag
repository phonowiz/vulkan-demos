
#version 450
#extension GL_ARB_separate_shader_objects: enable


layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_uv_coord;
layout(location = 0) out vec4 out_color;

//binding 0 is used in vertex shader
layout(binding = 1 ) uniform sampler2D normals;//albedo;
layout(binding = 2 ) uniform sampler2D albedo;
layout(binding = 3 ) uniform sampler2D world_positions;
layout(binding = 4 ) uniform sampler2D depth;

layout(binding = 5) uniform _rendering_state
{
    vec4 world_cam_position;
    //todo: figure out how to pass in an array of light positions
    vec4 world_light_position;
    vec4 light_color;
    int state;
}rendering_state;

layout(binding = 6) uniform _settings
{
    vec3 world_cam_position;
    

}settings;


//note: these are tied to enum class in deferred_renderer class, if these change, make sure
//make respective change accordingly

int ALBEDO = 0;
int NORMALS = 1;
int POSITIONS = 2;
int DEPTH = 3;
int FULL_RENDERING = 4;



vec4 direct_illumination(vec4 illumination)
{

    vec3 world_normal = texture(normals, frag_uv_coord).xyz;
    vec3 world_position = texture(world_positions, frag_uv_coord).xyz;
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
        //final composite shot will be assembled here
        out_color = direct_illumination(vec4(0.0f));
        //out_color = vec4(.5f, .5f, .5f, 1.0f);
    }
    //outColor = texture(depth, fragUVCoord );

    

}

