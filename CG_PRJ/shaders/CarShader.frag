#version 450

const float SHININESS = 150.0;
const float SPECULAR_INTENSITY = 0.5;
const float AMBIENT_INTENSITY = 0.1;

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject{
	vec3 lightPos; 
	vec4 lightColor; 
	vec3 viewerPosition; 
} gubo; 

layout(set = 1, binding = 1) uniform sampler2D carTexture;

layout(location = 0) in vec2 fragTexCoord; // Interpolated texture coordinate
layout(location = 1) in vec3 fragNormal; 
layout(location = 2) in vec3 fragPos; 

layout(location = 0) out vec4 outColor; // Output color

vec3 getLightDir_DL_M(vec3 lightPos) {
	return normalize(lightPos); 
}

vec3 getLightColor_DL_M(vec4 light_color) {
	return light_color.rgb * light_color.a; 
}

vec3 lambertDiffuse(vec3 lightDir, vec3 normal) {
	vec3 texColor = texture(carTexture, fragTexCoord).rgb;
	return texColor * (1 - AMBIENT_INTENSITY) *  max(dot(lightDir, normal), 0.0); 
}

vec3 blinnSpecular(vec3 lightDir, vec3 normal, vec3 viewerPos){
	vec3 viewer_direction = normalize(viewerPos - fragPos); 
	vec3 half_vector = normalize(lightDir + viewer_direction); 
	vec3 specular_color = vec3(1.0); 
	return specular_color * pow(max(dot(normal, half_vector), 0.0), SHININESS);
}

void main(){
	vec3 texColor = texture(carTexture, fragTexCoord).rgb; 
	vec3 light_color = getLightColor_DL_M(gubo.lightColor); 
	vec3 lightDir_DL = getLightDir_DL_M(gubo.lightPos); 
	vec3 ambient = texColor * AMBIENT_INTENSITY;
	vec3 sunColor = light_color * (lambertDiffuse(lightDir_DL, normalize(fragNormal)) + blinnSpecular(lightDir_DL, normalize(fragNormal), gubo.viewerPosition));
	outColor = vec4(ambient + sunColor, 1.0); 
}






