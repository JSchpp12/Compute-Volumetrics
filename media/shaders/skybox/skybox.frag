#version 450 core

layout(location = 0) in vec3 inFragDir;

layout(location = 0) out vec4 outColor;

layout(binding = 0, set = 2) uniform samplerCube skyboxSampler;

void main()
{
    outColor = vec4(texture(skyboxSampler, normalize(inFragDir)).rgb, 1.0);
}
