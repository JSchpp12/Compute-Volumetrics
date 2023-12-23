#version 450 core

layout (triangles_adjacency) in;
layout (line_strip) out;
layout (max_vertices = 2) out; 

in gl_PerVertex{
    vec4 gl_Position; 
    float gl_PointSize; 
    float gl_ClipDistance[];
    float gl_CullDistance[];
} gl_in[]; 

layout(location = 0) in vec3 normal[]; 

layout(binding = 0, set = 0) uniform GlobalUniformBufferObject {
	mat4 proj;
	mat4 view;  
	mat4 inverseView; 
	int numLights;
	int renderSettings; 
} globalUbo; 

const float MAG = 0.02; 

void projNormal(const int index){
    gl_Position = globalUbo.proj * gl_in[index].gl_Position; 
    EmitVertex(); 
    gl_Position = globalUbo.proj * (gl_in[index].gl_Position + (vec4(normal[index], 0.0) * MAG)); 
    EmitVertex(); 
    EndPrimitive(); 
}

void main(){
    //the verts provided HAVE to be triangles_adj 
    //only emit normals on actual verts for this triangle not neighbors
    projNormal(0);
    projNormal(2); 
    projNormal(4);
}
