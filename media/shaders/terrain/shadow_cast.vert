#version 450

layout(location = 0) in vec3 inPosition;

layout(binding = 0, set = 0) uniform GlobalUniformBufferObject
{
    mat4 viewProj;
}
globalUbo;

layout(binding = 0, set = 1) uniform instanceModelMatrix
{
    mat4 modelMatrix[1024];
};

void main()
{
    gl_Position = globalUbo.viewProj * modelMatrix[gl_InstanceIndex] * vec4(inPosition, 1.0);
}