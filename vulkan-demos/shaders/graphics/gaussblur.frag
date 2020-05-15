

#version 450
#extension GL_ARB_separate_shader_objects: enable

//based off of https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/bloom/gaussblur.frag
//Sascha Willems blur example

layout (binding = 0) uniform sampler2D samplerColor;

layout (binding = 1) uniform UBO
{
    float blurScale;
    float blurStrength;
    int   blurDirection;
} ubo;

layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main()
{
    
    
    float weight[5];
    weight[0] = 0.227027;
    weight[1] = 0.1945946;
    weight[2] = 0.1216216;
    weight[3] = 0.054054;
    weight[4] = 0.016216;

    vec2 tex_offset = 1.0 / textureSize(samplerColor, 0) * ubo.blurScale; // gets size of single texel
    vec3 result = texture(samplerColor, inUV).rgb * weight[0]; // current fragment's contribution
    for(int i = 1; i < 5; ++i)
    {
        if (ubo.blurDirection == 1)
        {
            // H
            result += texture(samplerColor, inUV + vec2(tex_offset.x * i, 0.0)).rgb * weight[i] * ubo.blurStrength;
            result += texture(samplerColor, inUV - vec2(tex_offset.x * i, 0.0)).rgb * weight[i] * ubo.blurStrength;
        }
        else
        {
            // V
            result += texture(samplerColor, inUV + vec2(0.0, tex_offset.y * i)).rgb * weight[i] * ubo.blurStrength;
            result += texture(samplerColor, inUV - vec2(0.0, tex_offset.y * i)).rgb * weight[i] * ubo.blurStrength;
        }
    }
    outFragColor = vec4(result, 1.0);
}
