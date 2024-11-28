#version 400
uniform mat4 PVM;
uniform mat4 M;
uniform vec3 viewPos; // Camera position

in vec3 pos_attrib;
in vec3 normal_attrib;

out vec3 fragPos;
out vec3 normal;
out vec3 viewDir;

void main() {
    fragPos = vec3(M * vec4(pos_attrib, 1.0));
    normal = mat3(transpose(inverse(M))) * normal_attrib;
    viewDir = viewPos - fragPos;
    gl_Position = PVM * vec4(pos_attrib, 1.0);
}
