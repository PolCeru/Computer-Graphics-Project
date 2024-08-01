#version 450

layout(set = 0, binding = 1) uniform UniformBufferObject {
	mat4 mvpMat;
	mat4 mMat;
	mat4 nMat;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal; 

layout(location = 0) out vec3 fragPos; 
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNorm; 

void main() {
	gl_Position = ubo.mvpMat * vec4(inPosition, 1.0);
	fragPos = (ubo.mMat * vec4(inPosition, 1.0)).xyz;
	fragTexCoord = inUV;
	fragNorm = (ubo.nMat * vec4(inNormal, 0.0)).xyz;
}