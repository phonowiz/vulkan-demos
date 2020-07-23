
#version 450
#extension GL_ARB_separate_shader_objects: enable


layout(location = 0) in vec2 in_frag_coord;
layout(location = 0) out vec4 out_color;

layout(binding = 0 ) uniform sampler2D color;

void main()
{
    out_color = texture(color, in_frag_coord);
}


