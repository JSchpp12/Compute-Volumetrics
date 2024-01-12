#version 450 

layout(location = 0) in vec2 inFragTextureCoordinate; 

layout(location = 0) out vec4 outColor;

layout(binding = 0, set = 2) uniform sampler2D textureSampler; 

void main() {
    outColor = vec4(texture(textureSampler, inFragTextureCoordinate));
}
