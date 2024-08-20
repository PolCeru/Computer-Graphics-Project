#version 450

const int MAP_SIZE = 23;


layout(set = 0, binding = 0) uniform GlobalUniformBufferObject{
	vec3 lightDir; 
	vec4 lightColorDirect; 
	vec3 viewerPosition; 
} gubo; 


layout(set = 1, binding = 0) uniform sampler2D floorTexture;

layout(set = 1, binding = 2) uniform RoadLightsUniformBufferObject{
	vec4 spotLight_lightPosition[MAP_SIZE * MAP_SIZE];
	vec4 spotLight_spotDirection[MAP_SIZE * MAP_SIZE];
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
	diffuse = texColor * max(dot(normal, lightDir), 0.0f); 
	
	//Blinn
	vec3 viewerDirection = normalize(viewerPosition - fragPos); 
	vec3 halfVector = normalize(lightDir + viewerDirection); 
	specular = vec3(pow(max(dot(normal, halfVector), 0.0), 40.0)); 

	return diffuse + specular; 
} 


void main() {

	vec4 i_sl_lightPos = rlubo.spotLight_lightPosition[current]; 
	vec4 i_sl_spotDir = rlubo.spotLight_spotDirection[current]; 
	vec4 i_sl_lightColor = rlubo.lightColorSpot; 
	
	vec3 texColor = texture(floorTexture, fragTexCoord).rgb; // Sample the texture
	vec3 normal = normalize(fragNorm);
	vec3 spotLight_lightDirection = normalize(fragPos - vec3(i_sl_lightPos)); 
	fragColor = vec4(i_sl_lightColor.rgb * pow(1.0 / length(fragPos - vec3(i_sl_lightPos)), 1.0) * clamp((dot(spotLight_lightDirection, vec3(i_sl_spotDir)) - 0.5) / 0.3, 0.0, 1.0), 1.0);
	//fragColor = vec4(vec3(i_sl_lightColor).rgb, 1.0); 
	fragColor += vec4(gubo.lightColorDirect.rgb * BRDF(texColor, normalize(gubo.lightDir), abs(normal), gubo.viewerPosition), 1.0f);
}