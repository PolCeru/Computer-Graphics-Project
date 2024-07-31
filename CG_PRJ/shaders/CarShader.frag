#version 450

layout(location = 0) in vec2 fragTexCoord; // Interpolated texture coordinate

layout(location = 0) out vec4 fragColor; // Output color

layout(binding = 1) uniform sampler2D floorTexture;

void main() {
    vec4 texColor = texture(floorTexture, fragTexCoord); // Sample the texture
    fragColor = texColor; // Assign the sampled color to the fragment
}
