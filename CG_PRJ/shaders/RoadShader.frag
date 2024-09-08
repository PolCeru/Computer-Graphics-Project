#version 450

// shader params
const float SHININESS = 150.0;
const float AMBIENT_INTENSITY = 0.2f;
const float SPECULAR_INTENSITY = 1.0f;
const int MAP_SIZE = 11;
const int NUM_CARS = 3;

// params for the car lights
const float G_CAR = 3.0f; 
const float BETA_CAR = 2.0f;
const float HEADLIGHT_INNER_CUTOFF = 1.0f;
const float HEADLIGHT_OUTER_CUTOFF = 0.3f;

// params for the road lights
const float G_LAMP = 6.0f;
const float BETA_LAMP = 3.0f;
const float LAMP_INNER_CUTOFF = 0.95f;
const float LAMP_OUTER_CUTOFF = 0.6f;

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

vec3 getLightDir_DL_M(vec3 lightPos) {
	return normalize(lightPos); 
}

vec3 getLightColor_DL_M(vec4 lightColor) {
	return lightColor.rgb * lightColor.a; 
}

vec3 getLightDir_SL_M(vec3 lightPos) {
	return normalize(lightPos - fragPos); 
}

vec3 getLightColor_SL_M(vec4 lightColor, vec3 lightPos, vec3 spotDirection, vec3 lightDirection, float g, float beta, float inner, float outer){
	return lightColor.rgb * lightColor.a * pow(g / length(fragPos - lightPos), beta) * clamp((dot(lightDirection, spotDirection) - outer) / (inner - outer), 0.0, 1.0); 
}

vec3 lambertDiffuse(vec3 lightDir, vec3 normal) {
	vec3 texColor = texture(floorTexture, fragTexCoord).rgb; 
	return texColor * (1 - AMBIENT_INTENSITY) * max(dot(lightDir, normal), 0.0); 
}

vec3 blinnSpecular(vec3 lightDir, vec3 normal, vec3 viewerPosition){
	vec3 viewer_direction = normalize(viewerPosition - fragPos); 
	vec3 half_vector = normalize(lightDir + viewer_direction); 
	vec3 specular_color = vec3(SPECULAR_INTENSITY); 
	return specular_color * pow(max(dot(normal, half_vector), 0.0), SHININESS);
}

void main() {
	vec3 texColor = texture(floorTexture, fragTexCoord).rgb; 
	vec3 lightDir_DL = getLightDir_DL_M(gubo.lightPos);
	vec3 lightDir_SL; 
	vec3 ambient = AMBIENT_INTENSITY * texColor;
	vec3 carsColor = vec3(0.0); 
	vec3 lampsColor = vec3(0.0); 
	vec3 normal = normalize(fragNorm);
	vec3 sunColor = getLightColor_DL_M(gubo.lightColor) * (lambertDiffuse(lightDir_DL, vec3(normal.x, abs(normal.y), normal.z))
																		+ blinnSpecular(lightDir_DL, vec3(normal.x, abs(normal.y), normal.z), gubo.viewerPosition));
	// Car lights
	for (int j = 0; j < NUM_CARS; j++) {
		for (int i = 0; i < 2; i++) {
			lightDir_SL = getLightDir_SL_M(cubo.headlightPosition[j][i]); 
			carsColor += getLightColor_SL_M(cubo.headlightColor[j][i], cubo.headlightPosition[j][i], cubo.headlightDirection[j][i], -lightDir_SL, G_CAR, BETA_CAR, HEADLIGHT_INNER_CUTOFF, HEADLIGHT_OUTER_CUTOFF) *
						 lambertDiffuse(lightDir_SL, vec3(normal.x, abs(normal.y), normal.z));  

			lightDir_SL = getLightDir_SL_M(cubo.rearLightPosition[j][i]);
			carsColor += getLightColor_SL_M(cubo.rearLightColor[j][i], cubo.rearLightPosition[j][i], cubo.rearLightDirection[j][i], -lightDir_SL, G_CAR, BETA_CAR, HEADLIGHT_INNER_CUTOFF, HEADLIGHT_OUTER_CUTOFF) * 
						lambertDiffuse(lightDir_SL, vec3(normal.x, abs(normal.y), normal.z));  
		}
	}

	// Road lights
	for(int i = 0; i < 3; i++) {
		lightDir_SL = getLightDir_SL_M(vec3(rlubo.spotLight_lightPosition[current][i])); 
		lampsColor += getLightColor_SL_M(rlubo.lightColorSpot, vec3(rlubo.spotLight_lightPosition[current][i]), vec3(rlubo.spotLight_spotDirection[current][i]), -lightDir_SL, G_LAMP, BETA_LAMP, LAMP_INNER_CUTOFF, LAMP_OUTER_CUTOFF) *  
						lambertDiffuse(lightDir_SL, vec3(normal.x, abs(normal.y), normal.z));  
	}

	fragColor = vec4(ambient + lampsColor + sunColor + carsColor, 1.0f);
}