#version 450

// Debug visualization of the precomputed transmittance map's texel grid.
// This is a copy of ../debugCube/debugCube.vert kept as its own file so the
// visualization object forms its own render-group pipeline (line-list
// topology) instead of sharing the filled debug cube's pipeline.

layout(location = 0) in vec3 inPosition;

layout(binding = 0, set = 0) uniform GlobalUniformBufferObject {
	mat4 proj;
	mat4 view;
	mat4 inverseView;
} globalUbo;

layout(binding = 0, set = 1) uniform instanceModelMatrix {
	mat4 modelMatrix[1024];
};

layout(binding = 0, set = 2) uniform instanceColors {
	vec4 colors[1024];
};

layout(location = 0) out vec3 outFragColor;

void main() {
	vec4 positionWorld = modelMatrix[gl_InstanceIndex] * vec4(inPosition, 1.0);
	gl_Position = globalUbo.proj * globalUbo.view * positionWorld;
	outFragColor = colors[gl_InstanceIndex].rgb;
}