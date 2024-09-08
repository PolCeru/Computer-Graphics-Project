#version 450

const float SHININESS = 150.0;
const float SPECULAR_INTENSITY = 0.5;
const float AMBIENT_INTENSITY = 0.15;

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject{
	vec3 lightDir; 
	vec4 lightColor; 
	vec3 viewerPosition; 
} gubo; 

layout(set = 1, binding = 1) uniform sampler2D floorTexture;

layout(location = 0) in vec3 fragPos; 
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNorm;

layout(location = 0) out vec4 fragColor; // Output color

vec3 lambertDiffuse(vec3 texColor, vec3 lightDir, vec3 normal) {
	vec3 diffuse; 
	diffuse = texColor * max(dot(lightDir, normal), 0.0f); 
	return diffuse; 
} 

void main() {
	vec3 texColor = texture(floorTexture, fragTexCoord).rgb; // Sample the texture
	vec3 normal = normalize(fragNorm);
	normal = vec3(normal.x, abs(normal.y), normal.z); 
	vec3 ambient = AMBIENT_INTENSITY * texColor;  
	
	vec3 finalColor = ambient + gubo.lightColor.rgb * gubo.lightColor.a * lambertDiffuse(texColor, normalize(gubo.lightDir), normal);
	fragColor = vec4((finalColor), 1.0f);
}