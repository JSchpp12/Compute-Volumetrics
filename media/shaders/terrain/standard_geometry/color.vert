#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(binding = 0, set = 0) uniform GlobalUniformBufferObject
{
    mat4 proj;
    mat4 view;
    mat4 inverseView;
}
globalUbo;

layout(binding = 0, set = 1) uniform instanceModelMatrix
{
    mat4 modelMatrix[1024];
};

layout(binding = 1, set = 1) uniform instanceNormalMatrix
{
    mat4 normalMatrix[1024];
};

layout(location = 0) out vec2 outFragTextureCoordinate;
layout(location = 1) out vec3 outFragPositionWorld;
layout(location = 2) out vec3 outFragNormalWorld;

void main()
{
    vec4 positionWorld = modelMatrix[gl_InstanceIndex] * vec4(inPosition, 1.0);

    gl_Position = globalUbo.proj * globalUbo.view * positionWorld;

    outFragNormalWorld = normalize(mat3(normalMatrix[gl_InstanceIndex]) * inNormal);
    outFragPositionWorld = positionWorld.xyz;
    outFragTextureCoordinate = inTexCoord;
}