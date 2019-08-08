
#version 450
#extension GL_ARB_separate_shader_objects: enable


layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_uv_coord;
layout(location = 0) out vec4 out_color;

//binding 0 is used in vertex shader
layout(binding = 1 ) uniform sampler2D albedo;
layout(binding = 2 ) uniform sampler2D normals;
layout(binding = 3 ) uniform sampler2D positions;
layout(binding = 4 ) uniform sampler2D depth;

layout(binding = 5) uniform RENDERING_STATE
{
    int state;
}s;


//note: these are tied to enum class in deferred_renderer class, if these change, make sure
//make respective change accordingly

int ALBEDO = 0;
int NORMALS = 1;
int POSITIONS = 2;
int DEPTH = 3;
int FULL_RENDERING = 4;


void main()
{
    if( s.state == ALBEDO )
    {
        out_color = texture(albedo, frag_uv_coord);
    }
    
    else if( s.state == NORMALS )
    {
        out_color = texture(normals, frag_uv_coord);
    }
    
    else if( s.state == POSITIONS)
    {
        out_color = texture(positions, frag_uv_coord);
    }
    else if( s.state == DEPTH )
    {
        out_color = texture(depth, frag_uv_coord );
        out_color.x = 1 - out_color.x;
        out_color.x *= 80.0f;
    }
    else
    {
        //final composite shot will be assembled here
        out_color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
    }
    //outColor = texture(depth, fragUVCoord );

    

}

