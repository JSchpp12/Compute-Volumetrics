#version 450

layout(location = 0) in vec3 inPosition;

layout(binding = 0, set = 0) uniform GlobalUniformBufferObject
{
    mat4 worldToLightViewProj;
    mat4 terrainToLightViewProj;
}
globalUbo;

layout(binding = 0, set = 1) uniform InstanceModelMatrix
{
    mat4 modelMatrix[1024];
}
instanceInfo;

void main()
{
    gl_Position = globalUbo.worldToLightViewProj * instanceInfo.modelMatrix[gl_InstanceIndex] * vec4(inPosition, 1.0);
}