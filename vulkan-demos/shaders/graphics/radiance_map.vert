
#version 450 core
#extension GL_ARB_separate_shader_objects: enable

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_uv_coord;
layout(location = 3) in vec3 in_normal;


void main()
{
    gl_Position = vec4(in_pos,1.0f);
}



