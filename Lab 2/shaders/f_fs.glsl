#version 400

uniform sampler2D diffuse_tex;   // Diffuse texture
uniform sampler2D normal_tex;    // Normal map texture

uniform vec3 lightPos[10];       // Light positions
uniform vec3 lightColor[10];     // Light colors
uniform float lightIntensity[10];// Light intensities
uniform int lightType[10];       // 0 = Point, 1 = Directional
uniform int lightCount;          // Number of active lights

uniform vec4 tintColor;          // Tint color
uniform float shininess;         // Shininess for specular highlights
uniform bool useTexture;         // Boolean to indicate if a texture is used
uniform bool useNormalMap;       // Boolean to indicate if a normal map is used

in vec3 fragPos;                 // Fragment position
in vec3 normal;                  // Interpolated normal
in vec2 tex_coord;               // Texture coordinates
in vec3 viewDir;                 // View direction

out vec4 fragcolor;              // Final fragment color

void main() {
    // Determine normal vector
    vec3 norm = normalize(normal);
    if (useNormalMap) {
        vec3 normalMapValue = texture(normal_tex, tex_coord).rgb;
        norm = normalize(normalMapValue * 2.0 - 1.0);
    }

    // Initialize lighting components
    vec3 ambient = vec3(0.0);
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    // Loop through all lights
    for (int i = 0; i < lightCount; i++) {
        // Calculate light direction based on type
        vec3 lightDir = (lightType[i] == 0) 
            ? normalize(lightPos[i] - fragPos) 
            : normalize(-lightPos[i]);

        // Ambient lighting (constant across the object)
        ambient += lightColor[i] * 0.1; // Adjust ambient intensity as needed

        // Diffuse lighting (based on angle between light and surface normal)
        float diff = max(dot(norm, lightDir), 0.0);
        diffuse += diff * lightColor[i] * lightIntensity[i];

        // Specular lighting (based on reflection and view direction)
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(normalize(viewDir), reflectDir), 0.0), shininess);
        specular += spec * lightColor[i] * lightIntensity[i];
    }

    // Combine lighting components
    vec3 lighting = ambient + diffuse + specular;

    // Apply texture only if it exists; otherwise, use white
    vec4 texColor = useTexture ? texture(diffuse_tex, tex_coord) : vec4(1.0);
    fragcolor = texColor * vec4(lighting, 1.0) * tintColor;
}
