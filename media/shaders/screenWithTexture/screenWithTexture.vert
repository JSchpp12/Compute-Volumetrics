#version 450 

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal; 
layout(location = 2) in vec3 inColor;		//vertex color
layout(location = 3) in vec2 inTexCoord;	//texture coordinate for vertex 
layout(location = 4) in vec3 inTangent; 
layout(location = 5) in vec3 inBiTangent; 
//per veterx materials
layout(location = 6) in vec3 inMatAmbient; 
layout(location = 7) in vec3 inMatDiffuse; 
layout(location = 8) in vec3 inMatSpecular; 
layout(location = 9) in float inMatShininess;  

layout(binding = 0, set = 0) uniform GlobalUniformBufferObject {
	mat4 proj;
	mat4 view;  
	mat4 inverseView; 
	int numLights;
} globalUbo; 

layout(binding = 0, set = 1) uniform instanceModelMatrix{
	mat4 modelMatrix[1024]; 
};

layout(binding = 1, set = 1) uniform instanceNormalMatrix{
	mat4 normalMatrix[1024];
}; 

layout(location = 0) out vec2 outFragTextureCoordinate; 

void main() {
	gl_Position = vec4(inPosition, 1.0);
	outFragTextureCoordinate = inTexCoord; 
}