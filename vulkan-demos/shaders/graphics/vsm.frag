#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(location = 0) out vec4 out_color;

void main()
{
    out_color.x =  gl_FragCoord.z;
    out_color.y = gl_FragCoord.z * gl_FragCoord.z;
    
    return;
}
