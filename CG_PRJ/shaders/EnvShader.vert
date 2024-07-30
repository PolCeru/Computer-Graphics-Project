#version 450

layout(binding = 0) uniform UniformBufferObject {
	mat4 mvpMat;
	mat4 mMat;
	mat4 nMat;
	vec4 color;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragTexCoord;

void main() {
	gl_Position = ubo.mvpMat * vec4(inPosition, 1.0);
	fragTexCoord = inUV;
}