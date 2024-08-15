#version 450

const float HEADLIGHT_INNER_CUTOFF = 0.9;
const float HEADLIGHT_OUTER_CUTOFF = 0.8;
const float SHININESS = 40.0;
const float SPECULAR_INTENSITY = 0.5;
const float AMBIENT_INTENSITY = 0.1;

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject{
	vec3 lightDir; 
	vec4 lightColor; 
	vec3 viewerPosition; 
} gubo; 

layout(set = 1, binding = 1) uniform sampler2D carTexture;

layout(set = 1, binding = 2) uniform CarLightsUniformBufferObject {
    vec3 headlightPosition[2];
    vec3 headlightDirection[2];
    vec4 headlightColor[2];

	vec3 rearLightPosition[2]; 
	vec3 rearLightDirection[2];
	vec4 rearLightColor[2];
};

layout(location = 0) in vec2 fragTexCoord; // Interpolated texture coordinate
layout(location = 1) in vec3 fragNormal; 
layout(location = 2) in vec3 fragPos; 

layout(location = 0) out vec4 outColor; // Output color

vec3 BRDF(vec3 texColor, vec3 lightDir, vec3 normal, vec3 viewerPosition) {
	vec3 diffuse, specular; 
	
	diffuse = texColor * max(dot(lightDir, normal), 0.0f);  //Lambert
	
	vec3 viewerDirection = normalize(viewerPosition - fragPos); 
	vec3 halfVector = normalize(lightDir + viewerDirection); 
	specular = SPECULAR_INTENSITY * vec3(pow(max(dot(normal, halfVector), 0.0), SHININESS)); //BLINN 
	
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
    vec3 texColor = texture(carTexture, fragTexCoord).rgb; // Sample the texture
	vec3 normal = normalize(fragNormal);
	vec3 ambient = AMBIENT_INTENSITY * texColor;  

	vec3 finalColor = ambient + gubo.lightColor.rgb * gubo.lightColor.a * BRDF(texColor, normalize(gubo.lightDir), abs(normal), gubo.viewerPosition);

	for (int i = 0; i < 2; i++) {
        finalColor += CalculateSpotlight(headlightPosition[i], headlightDirection[i], headlightColor[i], abs(normal), 1.0, 0.5);
    }

	 for (int i = 0; i < 2; i++) {
       finalColor += CalculateSpotlight(rearLightPosition[i], rearLightDirection[i], rearLightColor[i], abs(normal), 1.0, 0.5);
    }

	outColor = vec4(finalColor, 1.0);
}
