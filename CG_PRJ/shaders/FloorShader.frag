#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject{
	vec3 lightDir; 
	vec4 lightColor; 
	vec3 viewerPosition; 
} gubo; 

layout(location = 0) out vec4 outColor; // Output color

void main() {
	outColor = vec4(0.5, 0.5, 0.5, 1.0);
}