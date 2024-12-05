#version 400 core

layout(location = 0) in vec3 aPos; // Vertex position

out vec3 TexCoords; // Pass to fragment shader

uniform mat4 view;       // View matrix (with translation removed)
uniform mat4 projection; // Projection matrix

void main()
{
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}  
