#version 450


layout(set = 1, binding = 0) uniform UniformBufferObject {
	mat4 mvpMat;
	mat4 mMat;
	mat4 nMat;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal; 

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNorm; 
layout(location = 2) out vec3 fragPos; 

void main() {
	gl_Position = ubo.mvpMat * vec4(inPosition, 1.0);
	fragPos = (ubo.mMat * vec4(inPosition, 1.0)).xyz;
	fragTexCoord = inUV;
	fragNorm = mat3(ubo.nMat) * inNormal;	
}