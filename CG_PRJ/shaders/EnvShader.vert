#version 450


layout(set = 1, binding = 1) uniform RoadUniformBufferObject {
	mat4 mvpMat[5];
	mat4 mMat[5];
	mat4 nMat[5];
} srubo;



layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal; 

layout(location = 0) out vec3 fragPos; 
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNorm; 

void main() {
	int i = gl_InstanceIndex ;
	gl_Position = srubo.mvpMat[i] * vec4(inPosition, 1.0);
	fragPos = (srubo.mMat[i] * vec4(inPosition, 1.0)).xyz;
	fragTexCoord = inUV;
	fragNorm = mat3(srubo.nMat[i]) * inNormal;
}