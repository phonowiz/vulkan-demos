
#version 450


layout (location = 0) in vec2 in_uv_coord;
layout (location = 1) in vec4 in_color;

layout (location = 0) out vec4 out_albedo;

layout (binding = 2) uniform sampler2D albedos;


void main()
{
    out_albedo = texture(albedos, in_uv_coord);
    //out_albedo = vec4(in_uv_coord, 0.0f, 1.0f);
    if(out_albedo == vec4(0))
    {
        out_albedo = in_color;
    }
    
//    out_albedo = vec4(.2f, .2f, .2f, .5f);
}

