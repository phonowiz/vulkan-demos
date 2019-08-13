#version 450


layout (location = 0) in vec4 inNormal;
layout (location = 1) in vec4 inAlbedo;
layout (location = 2) in vec4 inWorldPos;

layout (location = 0) out vec4 outNormal;
layout (location = 1) out vec4 outAlbedo;
layout (location = 2) out vec4 outWorldPos;

void main()
{
//    outPosition = vec4(inWorldPos, 1.0);
//
//    // Calculate normal in tangent space
//    vec3 N = normalize(inNormal);
//    N.y = -N.y;
//    vec3 T = normalize(inTangent);
//    vec3 B = cross(N, T);
//    mat3 TBN = mat3(T, B, N);
//    vec3 tnorm = TBN * normalize(texture(samplerNormalMap, inUV).xyz * 2.0 - vec3(1.0));
//    outNormal = vec4(tnorm, 1.0);
//
//    outAlbedo = texture(samplerColor, inUV);
    
    outNormal = inNormal;
    outAlbedo = inAlbedo;
    outWorldPos = inWorldPos;
}
