#version 450

const float SHININESS = 10.0;
const float SPECULAR_INTENSITY = 0.5;
const float AMBIENT_INTENSITY = 0.3;

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

vec3 BRDF(vec3 texColor, vec3 lightDir, vec3 normal, vec3 viewerPosition) {
	vec3 diffuse, specular; 

	//Lambert
	diffuse = texColor * max(dot(lightDir, normal), 0.0f); 
	
	//Blinn
	vec3 viewerDirection = normalize(viewerPosition - fragPos); 
	vec3 halfVector = normalize(lightDir + viewerDirection); 
	specular = SPECULAR_INTENSITY * vec3(pow(max(dot(normal, halfVector), 0.0), SHININESS)); 

	return diffuse + specular; 
} 

void main() {
	vec3 texColor = texture(floorTexture, fragTexCoord).rgb; // Sample the texture
	vec3 normal = normalize(fragNorm);
	vec3 ambient = AMBIENT_INTENSITY * texColor;  

	/*vec3 finalColor = ambient + gubo.lightColor.rgb * gubo.lightColor.a * BRDF(texColor, normalize(gubo.lightDir), abs(normal), gubo.viewerPosition);

	fragColor = vec4(finalColor, 1.0f);

}