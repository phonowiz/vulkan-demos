
#version 450
#extension GL_ARB_separate_shader_objects: enable


layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUVCoord;
layout(location = 0) out vec4 outColor;

//binding 0 is used in vertex shader
layout(binding = 1 ) uniform sampler2D normals;
layout(binding = 2 ) uniform sampler2D albedo;
layout(binding = 3 ) uniform sampler2D positions;
layout(binding = 4 ) uniform sampler2D depth;

void main()
{
    //outColor = texture(depth, fragUVCoord );
    outColor = texture(albedo, fragUVCoord );

    outColor.x = 1 - outColor.x;
    outColor.x *= 80.0f;
    

}

