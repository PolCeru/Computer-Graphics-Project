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

vec3 getLightDir_directLightModel(vec3 lightPos) {
	return normalize(lightPos); 
}

vec4 getLightColor() {
	return gubo.lightColor; 
}

vec3 computeLambertDiffuse(vec3 lightDir) {
	vec3 texColor = texture(carTexture, fragTexCoord).rgb; 
	vec3 normal_vector = normalize(fragNormal); 
	return texColor * (1 - AMBIENT_INTENSITY) *  max(dot(light_dir, normal_vector), 0.0); 
}

vec3 computeBlinnSpecular(vec3 lightDir){
	vec3 normal_vector = normalize(fragNormal); 
	vec3 viewer_direction = normalize(gubo.viewerPosition - fragPos); 
	vec3 half_vector = normalize(light_dir + viewer_direction); 
	vec3 specular_color = vec3(1.0); 
	return specular_color * pow(max(dot(normal_vector, half_vector), 0.0), SHININESS);
}

void main(){
	vec3 texColor = texture(carTexture, fragTexCoord).rgb; 
	vec4 light_color = getLightColor(); 
	vec3 light_direction_DL = getLightDir_directLightModel(gubo.lightPos); 
	vec3 sunColor = AMBIENT_INTENSITY * texColor + light_color.rgb * light_color.a * (computeLambertDiffuse(light_direction_DL) + computeBlinnSpecular(light_direction_DL));
	outColor = vec4(sunColor, 1.0); 
}






