#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject{
	vec3 lightDir; 
	vec4 lightColor; 
	vec3 viewerPosition; 
} gubo; 

layout(set = 1, binding = 1) uniform sampler2D floorTexture;

layout(location = 0) in vec2 fragTexCoord; // Interpolated texture coordinate
layout(location = 1) in vec3 fragNormal; 
layout(location = 2) in vec3 fragPos; 

layout(location = 0) out vec4 outColor; // Output color


vec3 BRDF(vec3 texColor, vec3 lightDir, vec3 normal, vec3 viewerPostion) {
	vec3 diffuse; 
	vec3 specular; 
	diffuse = texColor * max(dot(lightDir, normal), 0.1f);  //Lambert
	
	vec3 viewerDirection = normalize(viewerPostion - fragPos); 
	vec3 halfVector = normalize(lightDir + viewerDirection); 
	specular = texColor * pow(max(dot(normal, halfVector), 0.0), 40.0); //BLINN 
	
	return diffuse; 
} 


void main() {
    vec3 texColor = texture(floorTexture, fragTexCoord).rgb; // Sample the texture
	vec3 normal = normalize(fragNormal);
	outColor = vec4(BRDF(texColor, gubo.lightDir, normal, gubo.viewerPosition), 1.0f);
}
