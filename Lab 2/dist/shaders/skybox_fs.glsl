#version 400 core

in vec3 TexCoords; // Interpolated texture coordinates
out vec4 FragColor; // Final color output

uniform samplerCube skybox; // Cubemap sampler

void main() {
    FragColor = texture(skybox, TexCoords); // Sample the cubemap

}