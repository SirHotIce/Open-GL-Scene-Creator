#version 400

uniform sampler2D diffuse_tex;       // Diffuse texture
uniform sampler2D normal_tex;        // Normal map texture

uniform vec3 lightPos[10];           // Light positions
uniform vec3 lightColor[10];         // Light colors
uniform float lightIntensity[10];    // Light intensities
uniform int lightType[10];           // 0 = Point, 1 = Directional
uniform int lightCount;              // Number of active lights

uniform vec4 tintColor;              // Tint color
uniform float shininess;             // Shininess for specular highlights
uniform bool useTexture;             // Boolean to indicate if a texture is used
uniform bool useNormalMap;           // Boolean to indicate if a normal map is used

uniform samplerCube environmentMap;  // HDR environment map
uniform float hdrIntensity;          // Intensity of the HDR reflection
uniform vec3 hdrRotation;            // Rotation of the HDR cubemap (optional)

in vec3 fragPos;                     // Fragment position
in vec3 normal;                      // Interpolated normal
in vec2 tex_coord;                   // Texture coordinates
in vec3 viewDir;                     // View direction (camera to fragment)

out vec4 fragcolor;                  // Final fragment color

// Function to sample HDR cubemap for lighting
vec3 sampleHDRLighting(vec3 norm) {
    // Sample cubemap environment map using the normal as a direction
    vec3 hdrColor = texture(environmentMap, norm).rgb * hdrIntensity;
    return hdrColor;
}

// Function to compute HDR reflection with shininess influence
vec3 computeHDRReflection(vec3 norm, vec3 viewDir, float shininess) {
    vec3 reflectedDir = reflect(-viewDir, norm);

    // Sample the cubemap environment map using the reflected direction
    vec3 hdrReflection = texture(environmentMap, reflectedDir).rgb;

    // Scale reflection by shininess (higher shininess => stronger reflection)
    float reflectionScale = clamp(pow(shininess / 10, 2.0), 0.0, 1.0);
    return hdrReflection * hdrIntensity * reflectionScale;
}

void main() {
    // Compute the surface normal
    vec3 norm = normalize(normal);
    if (useNormalMap) {
        vec3 normalMapValue = texture(normal_tex, tex_coord).rgb;
        norm = normalize(normalMapValue * 2.0 - 1.0);
    }

    // Initialize lighting components
    vec3 ambient = vec3(0.0);
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    // Add HDRI-based ambient lighting
    ambient += sampleHDRLighting(norm);

    // Add lighting from traditional light sources
    for (int i = 0; i < lightCount; i++) {
        vec3 lightDir = (lightType[i] == 0) 
            ? normalize(lightPos[i] - fragPos) 
            : normalize(-lightPos[i]);

        // Diffuse lighting from light sources
        float diff = max(dot(norm, lightDir), 0.0);
        diffuse += diff * lightColor[i] * lightIntensity[i];

        // Specular lighting from light sources
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(normalize(viewDir), reflectDir), 0.0), shininess);
        specular += spec * lightColor[i] * lightIntensity[i];
    }

    // Add HDRI-based reflection as specular contribution
    vec3 hdrReflection = computeHDRReflection(norm, viewDir, shininess);

    // Combine all lighting contributions
    vec3 lighting = ambient + diffuse + specular + hdrReflection;

    // Determine texture color or fallback to white if no texture is used
    vec4 texColor = useTexture ? texture(diffuse_tex, tex_coord) : vec4(1.0);

    // Final fragment color with tint and lighting
    fragcolor = texColor * vec4(lighting, 1.0) * tintColor;
}
