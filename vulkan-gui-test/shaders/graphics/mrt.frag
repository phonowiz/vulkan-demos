#version 450


layout (location = 0) in vec4 inNormal;
layout (location = 1) in vec4 inAlbedo;
layout (location = 2) in vec4 inWorldPos;

layout (location = 0) out vec4 outNormal;
layout (location = 1) out vec4 outAlbedo;
layout (location = 2) out vec4 outWorldPos;


//based off of: https://aras-p.info/texts/CompactNormalStorage.html and
//https://en.wikipedia.org/wiki/Lambert_azimuthal_equal-area_projection

vec2 encode (vec3 n)
{
    float f = sqrt(8.0f*n.z+8.0f);
    return n.xy / f + 0.5;
}

void main()
{
    outNormal.xyz = normalize(inNormal.xyz);
    outNormal.xy = encode( outNormal.xyz);
    outNormal.zw = vec2(0.0f,1.f);
    outAlbedo = inAlbedo;
    outWorldPos = inWorldPos;
}
