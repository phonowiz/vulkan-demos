#version 450
#extension GL_ARB_separate_shader_objects: enable

out gl_PerVertex
{
    vec4 gl_Position;
};

//vec2 positions[3] = vec2[](
//    vec2(0.0f, -0.5f),
//    vec2(0.5f, 0.5f),
//    vec2(-0.5f, 0.5f)
//);
////
//vec3 colors[3] = vec3[](
//    vec3(1.0f, 0.0f, 0.0f),
//    vec3(0.0f, 1.0f, 0.0f),
//    vec3(0.0f, 0.0f, 1.0f)
//);
layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform UBO
{
    mat4 MVP;
} ubo;


void main()
{
    gl_Position = ubo.MVP * vec4(pos, 0.0, 1.0f);//vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
    //gl_Position = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
    fragColor = color;//vec3(1.0f, 1.0f, 1.0f); //color;//colors[gl_VertexIndex];
    //fragColor = colors[gl_VertexIndex];
}


//#version 450
//#extension GL_ARB_separate_shader_objects : enable
//
//layout(location = 0) in vec2 inPosition;
//layout(location = 1) in vec3 inColor;
//
//layout(location = 0) out vec3 fragColor;
//
//void main() {
//    gl_Position = vec4(inPosition, 0.0, 1.0);
//    fragColor = inColor;
//}
