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

vec3 ON(vec3 texColor, vec3 lightDir, vec3 normal, vec3 viewerPosition) {
	float sigma_squared = pow(3.14, 2); 
	float theta_i = acos(dot(lightDir, normal)); 
	float theta_r = acos(dot(normalize(viewerPosition), normal)); 
	float alpha = max(theta_i, theta_r); 
	float beta = min(theta_i, theta_r); 
	float A = 1 - ((0.5 * sigma_squared) / (sigma_squared + 0.33)); 
	float B = (0.45 * sigma_squared) / (sigma_squared + 0.09); 
	vec3 vi = normalize(gubo.lightDir - dot(gubo.lightDir, normal) * normal); 
	vec3 vr = normalize(normalize(viewerPosition) - dot(normalize(viewerPosition), normal) * normal); 
	float G = max(0, dot(vi, vr)); 
	vec3 L = texColor * max(dot(lightDir, normal), 0.0f);
	return L * (A + B*G*sin(alpha)*tan(beta));
}

void main() {
	vec3 texColor = texture(floorTexture, fragTexCoord).rgb; // Sample the texture
	vec3 normal = normalize(fragNorm);
	vec3 ambient = AMBIENT_INTENSITY * texColor;  

	vec3 finalColor = ambient + gubo.lightColor.rgb * gubo.lightColor.a * ON(texColor, normalize(gubo.lightDir), abs(normal), gubo.viewerPosition);

	fragColor = vec4(finalColor, 1.0f);
}