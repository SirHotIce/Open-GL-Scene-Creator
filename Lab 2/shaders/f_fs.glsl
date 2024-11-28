#version 400

uniform vec3 lightPos[10]; // Support for 10 lights
uniform vec3 lightColor[10];
uniform float lightIntensity[10];
uniform int lightType[10]; // 0 = Point, 1 = Directional
uniform int lightCount;

uniform vec4 tintColor;
uniform float shininess;

in vec3 fragPos;
in vec3 normal;
in vec3 viewDir;

out vec4 fragColor;

void main() {
    vec3 ambient = vec3(0.1) * tintColor.rgb;
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    vec3 norm = normalize(normal);
    vec3 viewDirNorm = normalize(viewDir);

    for (int i = 0; i < lightCount; i++) {
        vec3 lightDir = (lightType[i] == 0) 
            ? normalize(lightPos[i] - fragPos) 
            : normalize(-lightPos[i]);

        float diff = max(dot(norm, lightDir), 0.0);
        diffuse += diff * lightColor[i] * lightIntensity[i];

        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDirNorm, reflectDir), 0.0), shininess);
        specular += spec * lightColor[i] * lightIntensity[i];
    }

    vec3 result = ambient + diffuse + specular;
    fragColor = vec4(result, 1.0);
}
