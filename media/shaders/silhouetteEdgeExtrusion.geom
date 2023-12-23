#version 450 

struct Light{
	vec4 position;
	vec4 direction;

	//properties
	vec4 ambient; 
	vec4 diffuse;
	vec4 specular; 
	//controls.x = inner cutoff angle 
	//controls.y = outer cutoff angle
	vec4 controls; 
	//settings.x = enabled
	//settings.y = type
	uvec4 settings; 
};

layout (triangles_adjacency) in;
layout (line_strip) out;
layout (max_vertices = 6) out; 

in gl_PerVertex{
    vec4 gl_Position; 
    float gl_PointSize; 
    float gl_ClipDistance[];
    float gl_CullDistance[];
} gl_in[]; 

layout(binding = 0, set = 0) uniform GlobalUniformBufferObject {
	mat4 proj;
	mat4 view;  
	mat4 inverseView; 
	int numLights;
	int renderSettings; 
} globalUbo; 

layout(binding = 1, set = 0) buffer globalLightBuffer{
    Light lights[];
};

const float DIST = 40.0f; 
const float DELTA = 0.000000001f; 

layout (location = 0) in vec3 worldPos[]; 


void emitLight(const int vertIndex, const vec3 lightPosition, const vec3 vertWorld)
{
    vec3 lightDir = lightPosition - vertWorld; 

    gl_Position = globalUbo.proj * gl_in[vertIndex].gl_Position;
    EmitVertex();

    gl_Position = globalUbo.proj * globalUbo.view * vec4(-lightDir * DIST, 1.0);  
    EmitVertex(); 

    // gl_Position = globalUbo.proj * gl_in[tIndex].gl_Position;
    // EmitVertex();

    EndPrimitive();
}

bool faceLight(const vec3 lightPosition, const vec3 edgeA, const vec3 edgeB, const vec3 edgeVert){
    vec3 normal = cross(edgeA, edgeB); 
    vec3 lightDir = lightPosition - edgeVert; 

    return dot(normal, lightDir) > DELTA;
}

bool isFront(const vec3 vertA, const vec3 vertB, const vec3 vertC){
    return 0 > (vertA.x * vertB.y - vertB.x * vertA.y) + (vertB.x * vertC.y - vertC.x * vertB.y) + (vertC.x * vertA.y - vertA.x * vertC.y); 
}

void main(){
    if (globalUbo.numLights > 0){
        Light curLight = lights[0]; 
        vec3 edgesWorld[6]; 
        edgesWorld[0] = worldPos[2] - worldPos[0]; 
        edgesWorld[1] = worldPos[4] - worldPos[0];
        edgesWorld[2] = worldPos[1] - worldPos[0]; 
        edgesWorld[3] = worldPos[3] - worldPos[2]; 
        edgesWorld[4] = worldPos[4] - worldPos[2];
        edgesWorld[5] = worldPos[5] - worldPos[0];
        
        vec3 vertScreen[6];
        vertScreen[0] = (gl_in[0].gl_Position).xyz;
        vertScreen[1] = (gl_in[1].gl_Position).xyz;  
        vertScreen[2] = (gl_in[2].gl_Position).xyz; 
        vertScreen[3] = (gl_in[3].gl_Position).xyz; 
        vertScreen[4] = (gl_in[4].gl_Position).xyz; 
        vertScreen[5] = (gl_in[5].gl_Position).xyz; 
        
        if (isFront(vertScreen[0], vertScreen[2], vertScreen[4])
            && faceLight(curLight.position.xyz, edgesWorld[0], edgesWorld[1], worldPos[0])){
            if (!isFront(vertScreen[0], vertScreen[1], vertScreen[2])
                && !faceLight(curLight.position.xyz, edgesWorld[2], edgesWorld[0], worldPos[0]))
                emitLight(0, curLight.position.xyz, worldPos[0]); 
            if (!isFront(vertScreen[2], vertScreen[3], vertScreen[4])
                && !faceLight(curLight.position.xyz, edgesWorld[3], edgesWorld[4], worldPos[2]))
                emitLight(2, curLight.position.xyz, worldPos[2]); 
            if (!isFront(vertScreen[4], vertScreen[5], vertScreen[0])
                && !faceLight(curLight.position.xyz, edgesWorld[1], edgesWorld[5], worldPos[4]))
                emitLight(4, curLight.position.xyz, worldPos[4]); 
        }
    } 
}