#version 450

layout(set = 0, binding = 2) uniform FloorUniformBufferObject{
	mat4 mvpMat; 
	mat4 mMat; 
	mat4 nMat; 
} fubo; 

layout(location = 0) in vec3 inPosition; 

void main() {
	gl_Position = fubo.mvpMat * vec4(inPosition, 1.0);
}