#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject{
	vec3 lightDir; 
	vec4 lightColor; 
	vec3 viewerPosition; 
} gubo; 

layout(set = 1, binding = 0) uniform sampler2D floorTexture;



layout(set = 1, binding = 3) uniform RoadLightsUniformBufferObject{
	vec4 lightColor; 
	float g; 
	float decayFactor; 
	float cosIn; 
	float cosOut; 
} road_light_ubo; 

layout(location = 0) in vec3 fragPos; 
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNorm;  
layout(location = 3) in vec3 fragLightPos; 
layout(location = 4) in vec3 fragSpotDir; 
layout(location = 5) in vec3 fragSpotPos; 


layout(location = 0) out vec4 fragColor; // Output color

vec3 BRDF(vec3 texColor, vec3 lightDir, vec3 normal, vec3 viewerPosition) {
	vec3 diffuse, specular; 

	//Lambert
	diffuse = texColor * max(dot(normal, lightDir), 0.0); 
	//diffuse = abs(texColor * dot(normal, lightDir)); 
	
	//Blinn
	vec3 viewerDirection = normalize(viewerPosition - fragPos); 
	vec3 halfVector = normalize(lightDir + viewerDirection); 
	specular = vec3(pow(max(dot(normal, halfVector), 0.0), 40.0)); 

	return diffuse + specular; 
} 


void main() {
    vec3 texColor = texture(floorTexture, fragTexCoord).rgb; // Sample the texture
	vec3 normal = normalize(fragNorm);
	//fragColor = vec4(gubo.lightColor.rgb * BRDF(texColor, normalize(gubo.lightDir), abs(normal), gubo.viewerPosition), 1.0f);
	float dist = length(fragLightPos - fragSpotPos);
	float distanceAttenuation = pow(road_light_ubo.g / dist, road_light_ubo.decayFactor);
	float spotEffect = dot(normalize(fragLightPos - fragSpotPos), normalize(fragSpotDir));
	float angularAttenuation = clamp((spotEffect - road_light_ubo.cosOut) / (road_light_ubo.cosIn - road_light_ubo.cosOut), 0.0, 1.0);
	float L = distanceAttenuation * angularAttenuation;
	fragColor = road_light_ubo.lightColor * L; 
}
