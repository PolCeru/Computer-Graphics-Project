#version 450

const int MAP_SIZE = 11;

layout(set = 1, binding = 1) uniform RoadUniformBufferObject {
	mat4 mvpMat[MAP_SIZE * MAP_SIZE];
	mat4 mMat[MAP_SIZE * MAP_SIZE];
	mat4 nMat[MAP_SIZE * MAP_SIZE];
} rubo;
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal; 
layout(location = 0) out vec3 fragPos; 
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNorm; 
layout(location = 3) out flat int current; 
void main() {
	int i = gl_InstanceIndex ;
	gl_Position = rubo.mvpMat[i] * vec4(inPosition, 1.0);
	fragPos = (rubo.mMat[i] * vec4(inPosition, 1.0)).xyz;
	fragTexCoord = inUV;
	fragNorm = mat3(rubo.nMat[i]) * inNormal;
	current = i; 
}