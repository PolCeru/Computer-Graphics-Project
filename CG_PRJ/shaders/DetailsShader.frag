#version 450


const float SHININESS = 150.0;
const float SPECULAR_INTENSITY = 1.0;
const float AMBIENT_INTENSITY = 0.15;

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject{
	vec3 lightPos; 
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
	diffuse = texColor * (1 - AMBIENT_INTENSITY) * max(dot(lightDir, normal), 0.0f); 
	return diffuse; 
} 

vec3 blinnSpecular(vec3 lightDir, vec3 normal, vec3 viewerPosition){
	vec3 viewer_direction = normalize(viewerPosition - fragPos); 
	vec3 half_vector = normalize(lightDir + viewer_direction); 
	vec3 specular_color = vec3(SPECULAR_INTENSITY);
	fragColor = vec4(lightDir, 1.0); 
	return specular_color * pow(dot(normal, half_vector), SHININESS);
}

vec3 phongSpecular(vec3 lightDir, vec3 normal, vec3 viewerPosition){
	vec3 viewer_direction = normalize(viewerPosition - fragPos); 
	return vec3(1.0) * (pow(max(dot(viewer_direction, -reflect(lightDir, normal)),0.0f), 150.0f)); 
}

void main() {
	vec3 texColor = texture(floorTexture, fragTexCoord).rgb; // Sample the texture
	vec3 normal = normalize(fragNorm);
	normal = vec3(normal.x, abs(normal.y), normal.z); 
	vec3 ambient = AMBIENT_INTENSITY * texColor;  
	vec3 finalColor =  ambient + gubo.lightColor.rgb * gubo.lightColor.a * (lambertDiffuse(texColor, gubo.lightPos, normal) + blinnSpecular(normalize(gubo.lightPos), normal, gubo.viewerPosition));
	//fragColor = vec4(phongSpecular(normalize(gubo.lightPos), normal, gubo.viewerPosition), 1.0f);
}