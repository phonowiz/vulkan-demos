

#version 450


layout (location = 0) in vec2 in_uv_coord;
layout (location = 1) in vec4 in_color;

layout (location = 0) out vec4 out_albedo;


void main()
{
    out_albedo = in_color;
    out_albedo.a = 1.0f;
}

