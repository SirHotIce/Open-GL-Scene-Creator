#version 400

uniform mat4 PVM;  // Projection-View-Model matrix
uniform mat4 M;    // Model matrix
uniform vec3 viewPos; // Camera position in world space

in vec3 pos_attrib;     // Vertex position
in vec3 normal_attrib;  // Vertex normal
in vec2 tex_coord_attrib; // Texture coordinates

out vec3 fragPos;      // Fragment position in world space
out vec3 normal;       // Normal vector
out vec2 tex_coord;    // Texture coordinates
out vec3 viewDir;      // View direction

void main() {
    // Transform vertex position into clip space
    gl_Position = PVM * vec4(pos_attrib, 1.0);

    // Transform vertex position into world space
    fragPos = vec3(M * vec4(pos_attrib, 1.0));

    // Transform normal into world space (accounting for non-uniform scaling)
    normal = mat3(transpose(inverse(M))) * normal_attrib;

    // Pass texture coordinates to the fragment shader
    tex_coord = tex_coord_attrib;

    // Calculate the view direction
    viewDir = viewPos - fragPos;
}
