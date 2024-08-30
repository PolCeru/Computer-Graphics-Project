#version 450

// shader params
const float SHININESS = 10.0;
const float SPECULAR_INTENSITY = 0.5;
const float AMBIENT_INTENSITY = 0.2;
const int MAP_SIZE = 11;

// params for the car lights
const float HEADLIGHT_INNER_CUTOFF = 1.0;
const float HEADLIGHT_OUTER_CUTOFF = 0.3;
// params for the road lights
const float G = 3.5;
const float BETA = 3.0;
const float SPOTLIGHT_INNER_CUTOFF = 0.95;
const float SPOTLIGHT_OUTER_CUTOFF = 0.6;
layout(set = 0, binding = 0) uniform GlobalUniformBufferObject{
	vec3 lightDir; 
	vec4 lightColor; 
	vec3 viewerPosition; 
} gubo; 
layout(set = 1, binding = 0) uniform sampler2D floorTexture;
layout(set = 1, binding = 2) uniform CarLightsUniformBufferObject {
    vec3 headlightPosition[2];
    vec3 headlightDirection[2];
    vec4 headlightColor[2];
	vec3 rearLightPosition[2]; 
	vec3 rearLightDirection[2];
	vec4 rearLightColor[2];
};
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
vec3 CalculateSpotlight(vec3 lightPos, vec3 lightDir, vec4 lightColor, vec3 normal, float innerCutoff, float outerCutoff) {
	vec3 lightVec = normalize(lightPos - fragPos);
	vec3 distance = normalize(-lightDir);
	float theta = dot(lightVec, distance);
	float intensity = clamp((theta - outerCutoff) / (innerCutoff - outerCutoff), 0.0, 1.0);
	vec3 diffuse = lightColor.rgb * lightColor.a * max(dot(lightVec, normal), 0.0);
	return intensity * diffuse;
}
void main() {
	//shader params
	vec3 texColor = texture(floorTexture, fragTexCoord).rgb; // Sample the texture
	vec3 normal = normalize(fragNorm);
	vec3 ambient = AMBIENT_INTENSITY * texColor;
	//Spotlight lamp params
	vec4 i_sl_lightPos; 
	vec4 i_sl_spotDir; 
	vec4 i_sl_lightColor = rlubo.lightColorSpot; 
	vec3 spotLight_lightDirection; 
	
	//Calculate the final color
	vec3 finalColor = ambient + gubo.lightColor.rgb * gubo.lightColor.a * BRDF(texColor, normalize(gubo.lightDir), abs(normal), gubo.viewerPosition);
    for (int i = 0; i < 2; i++) {
        finalColor += CalculateSpotlight(headlightPosition[i], headlightDirection[i], headlightColor[i], abs(normal), HEADLIGHT_INNER_CUTOFF, HEADLIGHT_OUTER_CUTOFF);
    }
	 for (int i = 0; i < 2; i++) {
       finalColor += CalculateSpotlight(rearLightPosition[i], rearLightDirection[i], rearLightColor[i], abs(normal), HEADLIGHT_INNER_CUTOFF, HEADLIGHT_OUTER_CUTOFF);
    }
	for(int i = 0; i < 3; i++) {
		i_sl_lightPos = rlubo.spotLight_lightPosition[current][i]; 
		i_sl_spotDir = rlubo.spotLight_spotDirection[current][i];
		vec3 spotLight_lightDirection = normalize(fragPos - vec3(i_sl_lightPos));
		finalColor += i_sl_lightColor.rgb * 
					  i_sl_lightColor.a *
					  pow(G / length(fragPos - vec3(i_sl_lightPos)), BETA) * clamp((dot(spotLight_lightDirection, vec3(i_sl_spotDir)) - SPOTLIGHT_OUTER_CUTOFF) / (SPOTLIGHT_INNER_CUTOFF - SPOTLIGHT_OUTER_CUTOFF), 0.0, 1.0);
	}
	
	fragColor = vec4(finalColor, 1.0f);
	}