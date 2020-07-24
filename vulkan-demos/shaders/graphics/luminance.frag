
#version 450
#extension GL_ARB_separate_shader_objects: enable


layout(location = 0) in vec2 in_frag_coord;
layout(location = 0) out vec4 out_color;

layout(binding = 0 ) uniform sampler2D color;



void main()
{
    //the assumption here is that the color coming in is not in gamma space yet, it is linear.
    
    vec4 linear = texture(color, in_frag_coord);
    
    //luminance
    float y = linear.x * 0.2126f +  linear.y * 0.7152f + linear.z * 0.0722f;
    
    out_color = vec4(vec3(y), 1.0f);
}


