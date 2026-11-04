#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBiTangent;
// per veterx materials
layout(location = 6) in vec3 inMatAmbient;
layout(location = 7) in vec3 inMatDiffuse;
layout(location = 8) in vec3 inMatSpecular;
layout(location = 9) in float inMatShininess;

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject
{
    mat4 proj;
    mat4 view;
    mat4 inverseView;
}
globalUbo;

layout(set = 1, binding = 0) uniform instanceModelMatrix
{
    mat4 modelMatrix[1024];
};

layout(set = 1, binding = 1) uniform instanceNormalMatrix
{
    mat4 normalMatrix[1024];
};

layout(set = 2, binding = 0) uniform instanceColors
{
    vec4 colors[1024];
};

layout(location = 0) out vec3 outFragDebugColor;
layout(location = 1) out vec3 outFragDirection;

void main()
{
    // skybox does not need translation
    const mat4 viewWithoutTranslation = mat4(mat3(globalUbo.view));
    vec4 pos = globalUbo.proj * viewWithoutTranslation * vec4(inPosition, 1.0);

    // force at far distance by setting z = w. Perspective divide will make this be 1 which is far clipping plane
    gl_Position = vec4(pos.x, pos.y, pos.w, pos.w);

    outFragDebugColor = vec3(colors[0]);
    outFragDirection = vec3(inPosition);
}