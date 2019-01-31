#version 450
#extension GL_ARB_separate_shader_objects: enable


layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUVCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1 ) uniform sampler2D tex;

void main()
{
    outColor = vec4(fragColor, .80f);
    //outColor = texture(tex, fragUVCoord );
    //outColor = vec4( gl_FragCoord.z *.2f, gl_FragCoord.z * .2f, gl_FragCoord.z* .2f, .50f);
}


//#version 450
//#extension GL_ARB_separate_shader_objects : enable
//
//layout(location = 0) in vec3 fragColor;
//
//layout(location = 0) out vec4 outColor;
//
//void main() {
//    outColor = vec4(fragColor, 1.0);
//}


//#version 450
//#extension GL_ARB_separate_shader_objects : enable
//
//layout(binding = 1) uniform sampler2D texSampler;
//
//layout(location = 0) in vec3 fragColor;
//layout(location = 1) in vec2 fragTexCoord;
//
//layout(location = 0) out vec4 outColor;
//
//void main() {
//    outColor = texture(texSampler, fragTexCoord);
//    //outColor = vec4( gl_FragCoord.z *.4f, gl_FragCoord.z * .4f, gl_FragCoord.z* .4f, .80f);
//}
