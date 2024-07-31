#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject{
	vec3 lightDir; 
	vec4 lightColor; 
	vec3 eyePos; 
} gubo; 

layout(location = 0) in vec2 fragTexCoord; // Interpolated texture coordinate
layout(location = 1) in vec3 fragNormal; 

layout(location = 0) out vec4 outColor; // Output color

layout(set = 1, binding = 1) uniform sampler2D floorTexture;

void main() {
    vec3 texColor = texture(floorTexture, fragTexCoord).rgb; // Sample the texture
	vec3 norm = normalize(fragNormal);
	outColor = vec4(texColor * max(dot(gubo.lightDir, norm), 0.0f), 1.0f);
}
