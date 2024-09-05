#version 450

// shader params
const float SHININESS = 150.0;
const float SPECULAR_INTENSITY = 0.5;
const float AMBIENT_INTENSITY = 0.2;
const int MAP_SIZE = 11;
const int NUM_CARS = 3;

// params for the car lights
const float G_CAR = 1.5; 
const float BETA_CAR = 1.2;
const float HEADLIGHT_INNER_CUTOFF = 1.0;
const float HEADLIGHT_OUTER_CUTOFF = 0.3;

// params for the road lights
const float G_LAMP = 3.5;
const float BETA_LAMP = 3.0;
const float LAMP_INNER_CUTOFF = 0.95;
const float LAMP_OUTER_CUTOFF = 0.6;

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject{
	vec3 lightPos; 
	vec4 lightColor; 
	vec3 viewerPosition; 
} gubo; 

layout(set = 1, binding = 0) uniform sampler2D floorTexture;

layout(set = 1, binding = 2) uniform CarLightsUniformBufferObject {
    vec3 headlightPosition[NUM_CARS][2];
    vec3 headlightDirection[NUM_CARS][2];
    vec4 headlightColor[NUM_CARS][2];

	vec3 rearLightPosition[NUM_CARS][2]; 
	vec3 rearLightDirection[NUM_CARS][2];
	vec4 rearLightColor[NUM_CARS][2];
} cubo;

layout(set = 1, binding = 3) uniform RoadLightsUniformBufferObject{
	vec4 spotLight_lightPosition[MAP_SIZE * MAP_SIZE][3];
	vec4 spotLight_spotDirection[MAP_SIZE * MAP_SIZE][3];
	vec4 lightColorSpot;
} rlubo; 

layout(location = 0) in vec3 fragPos; 
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNorm; 
layout(location = 3) in flat int current; 

layout(location = 0) out vec4 fragColor; // Output color

vec3 getLightDir_directLightModel(vec3 lightPos) {
	return normalize(lightPos); 
}

vec3 getLightColor_directLightModel(vec4 lightColor) {
	return lightColor.rgb * lightColor.a; 
}

vec3 getLightDir_spotLightModel(vec3 lightPos) {
	return normalize(lightPos - fragPos); 
}

vec3 getLightColor_spotLightModel(vec4 lightColor, vec3 lightPos, vec3 spotDirection, vec3 lightDirection, float g, float beta, float inner, float outer){
	return lightColor.rgb * lightColor.a * pow(g / length(fragPos - lightPos), beta) * clamp((dot(lightDirection, spotDirection) - outer) / (inner - outer), 0.0, 1.0); 
}

vec3 computeLambertDiffuse(vec3 lightDir, vec3 normal) {
	vec3 texColor = texture(floorTexture, fragTexCoord).rgb; 
	return texColor * (1 - AMBIENT_INTENSITY) *  max(dot(lightDir, normal), 0.0); 
}

vec3 computeBlinnSpecular(vec3 lightDir, vec3 normal, vec3 viewerPosition){
	vec3 viewer_direction = normalize(viewerPosition - fragPos); 
	vec3 half_vector = normalize(lightDir + viewer_direction); 
	vec3 specular_color = vec3(1.0); 
	return specular_color * pow(max(dot(normal, half_vector), 0.0), SHININESS);
}

void main() {
	vec3 texColor = texture(floorTexture, fragTexCoord).rgb; 
	vec3 light_direction_DL = getLightDir_directLightModel(gubo.lightPos);
	vec3 light_direction_SL; 
	vec3 sunColor = AMBIENT_INTENSITY * texColor + getLightColor_directLightModel(gubo.lightColor) * (computeLambertDiffuse(light_direction_DL, abs(normalize(fragNorm))) + computeBlinnSpecular(light_direction_DL, abs(normalize(fragNorm)), gubo.viewerPosition));
	vec3 carsColor = vec3(0.0); 
	vec3 lampsColor = vec3(0.0); 
	for (int j = 0; j < NUM_CARS; j++) {
		for (int i = 0; i < 2; i++) {
			light_direction_SL = getLightDir_spotLightModel(cubo.headlightPosition[j][i]); 
			carsColor += getLightColor_spotLightModel(cubo.headlightColor[j][i], cubo.headlightPosition[j][i], cubo.headlightDirection[j][i], -light_direction_SL, G_CAR, BETA_CAR, HEADLIGHT_INNER_CUTOFF, HEADLIGHT_OUTER_CUTOFF) * computeLambertDiffuse(light_direction_SL, normalize(fragNorm));  
			light_direction_SL = getLightDir_spotLightModel(cubo.rearLightPosition[j][i]);
			carsColor += getLightColor_spotLightModel(cubo.rearLightColor[j][i], cubo.rearLightPosition[j][i], cubo.rearLightDirection[j][i], -light_direction_SL, G_CAR, BETA_CAR, HEADLIGHT_INNER_CUTOFF, HEADLIGHT_OUTER_CUTOFF) * computeLambertDiffuse(light_direction_SL, normalize(fragNorm));  
		}
	}
	for(int i = 0; i < 3; i++) {
		light_direction_SL = getLightDir_spotLightModel(vec3(rlubo.spotLight_lightPosition[current][i])); 
		lampsColor += getLightColor_spotLightModel(rlubo.lightColorSpot, vec3(rlubo.spotLight_lightPosition[current][i]), vec3(rlubo.spotLight_spotDirection[current][i]), -light_direction_SL, G_LAMP, BETA_LAMP, LAMP_INNER_CUTOFF, LAMP_OUTER_CUTOFF);  
		computeLambertDiffuse(light_direction_SL, abs(normalize(fragNorm)));  
	}
	fragColor = vec4(lampsColor + sunColor, 1.0f);

}