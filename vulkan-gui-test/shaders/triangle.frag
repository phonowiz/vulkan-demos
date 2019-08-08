#version 450
#extension GL_ARB_separate_shader_objects: enable


layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUVCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragViewVec;
layout(location = 4) in vec3 fragLightVec;

//this is an attachment in the c++ side
layout(location = 0) out vec4 outColor;


layout(push_constant) uniform PushConstants
{
    bool usePhong;
}pushConts;

//this is part of the descriptor set, where binding 1 is for the the combined texture/image binding
layout(binding = 1 ) uniform sampler2D tex;

void main()
{
    //outColor = vec4(fragColor, .80f);
    //outColor = texture(tex, fragUVCoord );
    //outColor = vec4( gl_FragCoord.z *.2f, gl_FragCoord.z * .2f, gl_FragCoord.z* .2f, .50f);
    
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(fragLightVec);
    vec3 V = normalize( fragViewVec);
    vec3 R = reflect (-L,N);
    
    vec4 ambient = fragColor * 0.1f;
    vec4 diffuse = max(dot(N,L), 0.0f) * fragColor;
    vec3 specular = pow(max(dot(R,V), 0.0f), 16.0f) * vec3(1.35f, 1.35f, 1.35f);
    
    
    outColor = vec4(ambient.xyz + diffuse.xyz + specular.xyz, 1.0f);
}



