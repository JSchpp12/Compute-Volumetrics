#version 450 core

layout(location = 0) in vec3 inFragDebugColor;
layout(location = 1) in vec3 inFragDir;

layout(location = 0) out vec4 outColor;

#define PI 3.14159265

void main()
{
    outColor = vec4(inFragDebugColor, 1.0);
}
