#version 450
#extension GL_ARB_separate_shader_objects: enable

layout(location = 0) out vec2 out_color;


////variance shadow maps, based off of
////http://developer.download.nvidia.com/SDK/10/direct3d/Source/VarianceShadowMapping/Doc/VarianceShadowMapping.pdf
////and
////http://www.punkuser.net/vsm/vsm_paper.pdf
//
//vec2 vsm_filter( vec2 moments, float fragDepth )
//{
// vec2 lit = vec2(0.0f);
// float E_x2 = moments.y;
// float Ex_2 = moments.x * moments.x;
// float variance = E_x2 - Ex_2;
// float mD = moments.x - fragDepth;
// float mD_2 = mD * mD;
// float p = variance / (variance + mD_2);
// lit.x = max( p, fragDepth <= moments.x );
//
// return lit; //lit.x == VSM calculation
//}



void main()
{
    out_color.x =  gl_FragCoord.z;
    out_color.y = gl_FragCoord.z * gl_FragCoord.z;
    
    return;
}
