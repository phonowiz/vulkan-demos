#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(location = 0) out vec4 out_color;

void main()
{
    float depth = gl_FragCoord.z;
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    
    float moments2 = depth * depth + 0.25f * (dx * dx + dy * dy);
    
    out_color.xy = vec2(depth, moments2);
    out_color.zw = vec2(0.0f, 1.0f);
//    out_color.x = gl_FragCoord.z;  //(1 - gl_FragCoord.z) * 90.0f;
//    out_color.y = gl_FragCoord.z * gl_FragCoord.z;
//    out_color.z = 0.0f;
//    out_color.w = 1.0f;
    
    //return;
}
