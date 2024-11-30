#version 400

in vec3 texCoords;
out vec4 fragColor;

uniform samplerCube skybox;
uniform float hdrIntensity;

void main() {
    vec3 hdrColor = texture(skybox, texCoords).rgb * hdrIntensity;
    fragColor = vec4(hdrColor, 1.0);
}
