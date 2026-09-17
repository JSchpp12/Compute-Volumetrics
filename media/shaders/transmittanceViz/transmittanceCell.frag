#version 450

// Debug visualization of the precomputed transmittance map's texel grid.
// Copy of ../debugCube/debugCube.frag (see the .vert note for why this file
// exists separately).

layout(location = 0) in vec3 inFragColor;
layout(location = 0) out vec4 outFragColor;

void main() {
	outFragColor = vec4(inFragColor, 1.0);
}