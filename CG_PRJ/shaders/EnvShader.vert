#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
	vec4 color;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragTexCoord;

void main() {
	fragTexCoord = inUV;
    gl_Position = rotate((ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0)), radians(180.0f), vec3(0, 1, 0));
}