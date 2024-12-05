#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include <windows.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "DebugCallback.h"
#include "ImGuiFileDialog.h"
#include "InitShader.h"
#include "LoadMesh.h"
#include "LoadTexture.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <random>
#include <stb/stb_image.h>
#include "FinalProject.h"

float vertices[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};
std::vector<std::string> cubemapPaths = {
    "", // Right
    "", // Left
    "", // Top
    "", // Bottom
    "", // Front
    ""  // Back
};

GLFWwindow* currentContext;
bool clearScreen = true;
bool enableDepthTest = true;
glm::vec3 camera_position = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 camera_rotation(0.0f, 0.0f, 0.0f);  // Camera rotation 
float camera_fov = 60.0f;
float near_clip = 0.1f;
float far_clip = 100.0f;

int window_width = 2000;
int window_height = 1200;
float aspect_ratio = 1.0f;

glm::vec3 forward(0.0f, 0.0f, -1.0f);  // Initial forward direction (looking along the negative Z axis)
glm::vec3 up(0.0f, 1.0f, 0.0f);      // Up direction
glm::vec3 right(1.0f, 0.0f, 0.0f);    // Right direction

glm::vec3 cameraForward(0.0f, 0.0f, -1.0f);  // Initial forward direction (looking along the negative Z axis)
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);      // Up direction
glm::vec3 cameraRight(1.0f, 0.0f, 0.0f);    // Right direction

GLuint shaderProgram = -1;
GLuint skyboxShaderProgram = -1;

struct ImportedMesh {
    int id; // Unique identifier
    std::string name;
    std::string meshFile;
    std::string textureFile;
    std::string normalMapFile;
    bool hasTexture = false;
    bool hasNormalMap = false;
    glm::vec4 tintColor = glm::vec4(1.0f);
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    float shininess = 3.0f; // Default shininess
    MeshData mesh;
    GLuint textureID = -1;
    GLuint normalMapID = -1;
};

std::vector<ImportedMesh> importedMeshes;
char meshNameInput[256] = "";
std::string meshFileInput = "";
std::string textureFileInput = "";
std::string normalMapFileInput = "";
bool enableTexture = false;
bool enableNormalMap = false;
glm::vec4 meshTintColor(1.0f);
glm::vec3 cubemapRotation = glm::vec3(0.0f);


struct Light {
    int id; // Unique identifier
    std::string type; // "Point" or "Directional"
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); // For directional light
    glm::vec3 color = glm::vec3(1.0f); // RGB
    float intensity = 0.5f;
};

std::vector<Light> lights;
int selectedLightType = 0; // 0 = Point, 1 = Directional
const char* lightTypes[] = { "Point", "Directional" };
GLuint skyboxVAO, skyboxVBO, cubemapTexture;
float cubeMapIntensity=1;

// Declare a global variable to track the first-person mode status
bool firstPersonMode = false;
bool isWindowed = false;
bool isFullScreen = false;

/// <summary>
/// the programm starts here
/// </summary>
/// <returns></returns>

int generate_unique_id() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, 1000000);
    return dist(rng);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    window_width = 2000;
    window_height = 1200;
    glViewport(0, 0, width, height);
    aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
}

void setFullScreen(GLFWwindow* window) {

    // Get the primary monitor's resolution
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);


    // Set the window to fullscreen
    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void restoreWindowed(GLFWwindow* window) {
    // Restore the window size and position
    glfwSetWindowMonitor(window, NULL, 50, 50, window_width, window_width, GLFW_DONT_CARE);
    glfwSetWindowSize(window, window_width, window_height);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);



}


glm::mat4 getViewMatrix() {
    glm::mat4 rotationMatrix = glm::mat4(1.0f);  // Identity matrix

    // Apply rotations in order: yaw (Y-axis), pitch (X-axis), roll (Z-axis)
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(camera_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));  // Yaw
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(camera_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));  // Pitch
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(camera_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));  // Roll

    cameraForward = -glm::normalize(glm::vec3(rotationMatrix[2])); // OpenGL forward is negative Z
    cameraRight = glm::normalize(glm::vec3(rotationMatrix[0]));    // X-axis of rotation matrix
    cameraUp = glm::normalize(glm::vec3(rotationMatrix[1]));       // Y-axis of rotation matrix

    return glm::lookAt(camera_position, camera_position + cameraForward, cameraUp);
}

glm::mat4 getProjectionMatrix(float aspect_ratio) {
    return glm::perspective(glm::radians(camera_fov), aspect_ratio, near_clip, far_clip);
}


void moveCamera(glm::vec3 movement) {
    camera_position += movement;
}

void rotateCamera(glm::vec3 rotation) {
    camera_rotation += rotation;
    camera_rotation.x = glm::clamp(camera_rotation.x, -89.0f, 89.0f); 
}



bool testFileRead(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filepath << std::endl;
        return false;
    }
    std::cout << "File read successfully: " << filepath << std::endl;
    return true;
}
GLuint cubeVAO = 0;
GLuint cubeVBO = 0;

void generateDummyFace(GLenum face, unsigned char r, unsigned char g, unsigned char b) {
    unsigned char color[3] = { r, g, b }; // RGB color
    glTexImage2D(
        face, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, color
    );
}
void initSkyBoxShader() {
    std::string vs = "shaders/skybox_vs.glsl";
    std::string fs = "shaders/skybox_fs.glsl";

    if (skyboxShaderProgram == -1) {
        skyboxShaderProgram = InitShader(vs.c_str(), fs.c_str());
    }
    else {
        glDeleteProgram(skyboxShaderProgram);
        skyboxShaderProgram = InitShader(vs.c_str(), fs.c_str());

    }
    GLint isLinked = 0;
    glGetProgramiv(skyboxShaderProgram, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE) {
        GLint maxLength = 0;
        glGetProgramiv(skyboxShaderProgram, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(skyboxShaderProgram, maxLength, &maxLength, &infoLog[0]);
        std::cerr << "Skybox Shader Linking Error:\n" << infoLog.data() << std::endl;

        glDeleteProgram(skyboxShaderProgram);
        return;
    }

}

unsigned int loadCubemapWithFallback(const std::vector<std::string>& paths) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    // Set cubemap texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < 6; ++i) {
        if (paths[i].empty()) {
            unsigned char r = static_cast<unsigned char>(170);
            unsigned char g = static_cast<unsigned char>(217); 
            unsigned char b = static_cast<unsigned char>(230); 
            generateDummyFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, r, g, b);
        }
        else {
            unsigned char* data = stbi_load(paths[i].c_str(), &width, &height, &nrChannels, 0);
            if (data) {
                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
                );
                stbi_image_free(data);
            }
            else {
                std::cerr << "Failed to load cubemap texture at path: " << paths[i] << std::endl;
                generateDummyFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 255, 0, 0);
            }
        }
    }

    return textureID;
}

void initSkybox(const std::vector<std::string>& cubemapPaths) {
    // Load the cubemap
    cubemapTexture = loadCubemapWithFallback(cubemapPaths);
    if (cubemapTexture == 0) {
        std::cerr << "Failed to load textures for skybox!" << std::endl;
        return;
    }
    
    GLint success;
    glGetProgramiv(skyboxShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(skyboxShaderProgram, 512, nullptr, infoLog);
        std::cerr << "Error linking skybox shader program: " << infoLog << std::endl;
        return;
    }
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxShaderProgram);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void renderSkybox() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glUseProgram(skyboxShaderProgram);

    glm::mat4 viewMatrix = getViewMatrix();
    glm::mat4 projectionMatrix = getProjectionMatrix(aspect_ratio);
    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(viewMatrix)); // Remove translation
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(viewNoTranslation));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    // Bind and render the skybox
    glBindVertexArray(skyboxVAO);
   

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    GLint boundCubemap;
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &boundCubemap);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glCullFace(GL_BACK);
    glDepthMask(GL_TRUE);

    
}

void ExitFSMode()
{
    restoreWindowed(currentContext);
    isWindowed = true;
    isFullScreen = false;
    firstPersonMode = false;
}

void drawCameraOptions() {
    ImGui::SetNextWindowPos(ImVec2(10, 850), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always);
    ImGui::Begin("Camera Options");
    ImGui::Text("FP Controls: W-Forward, S- Back, A- Left, D- Right.");
    ImGui::Text(", Q-Up, E-Down, Mouse-LookAround, ESC- Close Full Screen");
    // Toggle for First Person Mode
    if (ImGui::Button(firstPersonMode ? "Exit First Person Mode" : "Enter First Person Mode")) {
        firstPersonMode = !firstPersonMode;

    }

    if (!firstPersonMode) {
        
        if (!isWindowed) {
            ExitFSMode();
        }
    }
    else {
        if(!isFullScreen) {
            setFullScreen(currentContext);
            isWindowed = false;
            isFullScreen = true;
        }
    }
    ImGui::Text("Camera Position");
    ImGui::DragFloat3("Position", &camera_position[0],1, -100.0f, 100.0f);
    ImGui::Text("Camera Rotation");
    ImGui::DragFloat3("Rotation", &camera_rotation[0],1,-180.0f, 180.0f);

    ImGui::Text("Camera FOV");
    ImGui::DragFloat("FOV", &camera_fov, 30.0f, 120.0f);

    ImGui::Text("Near/Far Clip");
    ImGui::DragFloat("Near", &near_clip, 0.1f, 100.0f);
    ImGui::DragFloat("Far", &far_clip, 100.0f, 1000.0f);

    ImGui::End();
}




void draw_mesh_import_window() {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always);
    ImGui::Begin("Import Mesh", nullptr, ImGuiWindowFlags_NoResize);

    if (meshNameInput[0] == '\0') {
        strncpy_s(meshNameInput, sizeof(meshNameInput), "Untitled Object", _TRUNCATE);
    }

    ImGui::InputText("Mesh Name", meshNameInput, sizeof(meshNameInput));

    IGFD::FileDialogConfig fileDialogConfig;
    fileDialogConfig.path = ".";
    fileDialogConfig.fileName = "";
    fileDialogConfig.countSelectionMax = 1;
    fileDialogConfig.flags = ImGuiFileDialogFlags_None;

    if (ImGui::Button("Select Mesh File")) {
        ImGuiFileDialog::Instance()->OpenDialog("ChooseMeshFile", "Select Mesh File", ".fbx,.obj,.dae", fileDialogConfig);
    }
    ImGui::SameLine();
    ImGui::Text("%s", meshFileInput.c_str());

    if (ImGui::Button("Select Texture File")) {
        ImGuiFileDialog::Instance()->OpenDialog("ChooseTextureFile", "Select Texture File", ".png,.jpg,.bmp", fileDialogConfig);
    }
    ImGui::SameLine();
    ImGui::Text("%s", textureFileInput.c_str());

    if (ImGui::Button("Select Normal Map File")) {
        ImGuiFileDialog::Instance()->OpenDialog("ChooseNormalMapFile", "Select Normal Map File", ".png,.jpg,.bmp", fileDialogConfig);
    }
    ImGui::SameLine();
    ImGui::Text("%s", normalMapFileInput.c_str());

    ImGui::Checkbox("Enable Texture", &enableTexture);
    ImGui::Checkbox("Enable Normal Map", &enableNormalMap);
    ImGui::ColorEdit4("Mesh Tint", glm::value_ptr(meshTintColor));

    if (ImGui::Button("Import Mesh") && meshFileInput != "") {
        ImportedMesh newMesh;
        newMesh.id = generate_unique_id();
        newMesh.name = meshNameInput;
        newMesh.meshFile = meshFileInput;
        newMesh.hasTexture = enableTexture;
        newMesh.hasNormalMap = enableNormalMap;
        newMesh.tintColor = meshTintColor;

        newMesh.mesh = LoadMesh(meshFileInput);

        if (enableTexture && !textureFileInput.empty()) {
            newMesh.textureID = LoadTexture(textureFileInput);
        }

        if (enableNormalMap && !normalMapFileInput.empty()) {
            newMesh.normalMapID = LoadTexture(normalMapFileInput);
        }

        importedMeshes.push_back(newMesh);
    }

    ImGui::End();

    if (ImGuiFileDialog::Instance()->Display("ChooseMeshFile", 32, ImVec2(500, 500), ImVec2(800, 800))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            meshFileInput = ImGuiFileDialog::Instance()->GetFilePathName();
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseTextureFile", 32, ImVec2(500, 500), ImVec2(800, 800))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            textureFileInput = ImGuiFileDialog::Instance()->GetFilePathName();
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseNormalMapFile", 32, ImVec2(500, 500), ImVec2(800, 800))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            normalMapFileInput = ImGuiFileDialog::Instance()->GetFilePathName();
        }
        ImGuiFileDialog::Instance()->Close();
    }
}


void drawCubeMapWindow() {
    ImGui::SetNextWindowPos(ImVec2(10, 540), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always);
    ImGui::Begin("Cubemap Management", nullptr, ImGuiWindowFlags_NoResize);

    static std::vector<std::string> tempCubemapPaths(6, ""); // Temporary paths for input
    static bool allFacesSelected = false;

    IGFD::FileDialogConfig fileDialogConfig;
    fileDialogConfig.path = ".";
    fileDialogConfig.fileName = "";
    fileDialogConfig.countSelectionMax = 1;
    fileDialogConfig.flags = ImGuiFileDialogFlags_None;

    const char* faceLabels[6] = { "Right", "Left", "Top", "Bottom", "Front", "Back" };

    for (int i = 0; i < 6; ++i) {
        ImGui::Text("%s Face:", faceLabels[i]);
        ImGui::SameLine();
        if (ImGui::Button(("Import " + std::string(faceLabels[i])).c_str())) {
            ImGuiFileDialog::Instance()->OpenDialog(("Choose " + std::string(faceLabels[i]) + " File").c_str(),
                "Select Face File", ".bmp,.png,.jpg", fileDialogConfig);
        }
        ImGui::SameLine();
        ImGui::Text("%s", tempCubemapPaths[i].empty() ? "Not Selected" : tempCubemapPaths[i].c_str());
    }

    for (int i = 0; i < 6; ++i) {
        if (ImGuiFileDialog::Instance()->Display(("Choose " + std::string(faceLabels[i]) + " File").c_str(),
            32, ImVec2(500, 500), ImVec2(800, 800))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                tempCubemapPaths[i] = ImGuiFileDialog::Instance()->GetFilePathName();
            }
            ImGuiFileDialog::Instance()->Close();
        }
    }

    // Check if all faces are selected
    allFacesSelected = std::all_of(tempCubemapPaths.begin(), tempCubemapPaths.end(),
        [](const std::string& path) { return !path.empty(); });

    if (allFacesSelected) {
        ImGui::Text("All faces selected. Ready to import.");
    }
    else {
        ImGui::Text("Please select all faces to import the cubemap.");
    }

    // Import button
    if (ImGui::Button("Import Cubemap") && allFacesSelected) {
        cubemapPaths = tempCubemapPaths;
        cubemapTexture = loadCubemapWithFallback(cubemapPaths);
        std::cout << "Cubemap imported successfully!" << std::endl;
        initSkyBoxShader();

    }

    // Clear button
    if (ImGui::Button("Clear Cubemap")) {
        tempCubemapPaths = std::vector<std::string>(6, ""); 
        cubemapPaths.clear(); 
        cubemapTexture = 0; 
        std::cout << "Cubemap cleared." << std::endl;
    }

    // Intensity slider
    static float intensity = 1.0f;
    ImGui::SliderFloat("Cubemap Intensity", &intensity, 0.1f, 2.0f);
    cubeMapIntensity = intensity;

    ImGui::End();
}


void draw_hierarchy_window() {
    ImGui::SetNextWindowPos(ImVec2(1590, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_Always);
    ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoResize);
    IGFD::FileDialogConfig fileDialogConfig;
    fileDialogConfig.path = ".";
    fileDialogConfig.fileName = "";
    fileDialogConfig.countSelectionMax = 1;
    fileDialogConfig.flags = ImGuiFileDialogFlags_None;

    for (size_t i = 0; i < importedMeshes.size(); ++i) {
        ImportedMesh& mesh = importedMeshes[i];
        std::string headerLabel = mesh.name + " (ID: " + std::to_string(mesh.id) + ")";
        if (ImGui::CollapsingHeader(headerLabel.c_str())) {
            ImGui::PushID(mesh.id);

            ImGui::DragFloat3("Position", glm::value_ptr(mesh.position), 0.1f);
            ImGui::DragFloat3("Rotation (Roll, Pitch, Yaw)", glm::value_ptr(mesh.rotation), 0.1f, -6.28, 6.28);
            ImGui::DragFloat3("Scale", glm::value_ptr(mesh.scale), 0.1f);
            ImGui::ColorEdit4("Tint", glm::value_ptr(mesh.tintColor));
            ImGui::DragFloat("Shininess", &mesh.shininess, .1f, 0.1f, 10.0f);

            // Add texture change feature
            static char textureBuffer[512];
            strncpy_s(textureBuffer, mesh.textureFile.c_str(), sizeof(textureBuffer));
            if (ImGui::Button("Change Texture")) {
                ImGuiFileDialog::Instance()->OpenDialog("ChangeTexture", "Select Texture File", ".png,.jpg,.bmp",fileDialogConfig);
            }
            ImGui::SameLine();
            ImGui::Text("%s", mesh.textureFile.c_str());
            if (ImGuiFileDialog::Instance()->Display("ChangeTexture", 32, ImVec2(500, 500), ImVec2(800, 800))) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    mesh.textureFile = ImGuiFileDialog::Instance()->GetFilePathName();
                    mesh.textureID = LoadTexture(mesh.textureFile); // Reload the texture
                    mesh.hasTexture = true;
                }
                ImGuiFileDialog::Instance()->Close();
            }

            // Add normal map change feature
            static char normalMapBuffer[512];
            strncpy_s(normalMapBuffer, mesh.normalMapFile.c_str(), sizeof(normalMapBuffer));
            if (ImGui::Button("Change Normal Map")) {
                ImGuiFileDialog::Instance()->OpenDialog("ChangeNormalMap", "Select Normal Map File", ".png,.jpg,.bmp", fileDialogConfig);
            }
            ImGui::SameLine();
            ImGui::Text("%s", mesh.normalMapFile.c_str());
            if (ImGuiFileDialog::Instance()->Display("ChangeNormalMap", 32, ImVec2(500, 500), ImVec2(800, 800))) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    mesh.normalMapFile = ImGuiFileDialog::Instance()->GetFilePathName();
                    mesh.normalMapID = LoadTexture(mesh.normalMapFile); // Reload the normal map
                    mesh.hasNormalMap = true;
                }
                ImGuiFileDialog::Instance()->Close();
            }

            // Toggles for using texture and normal map
            ImGui::Checkbox("Use Texture", &mesh.hasTexture);
            ImGui::Checkbox("Use Normal Map", &mesh.hasNormalMap);


            if (ImGui::Button("Delete")) {
                importedMeshes.erase(importedMeshes.begin() + i);
                ImGui::PopID();
                break;
            }

            ImGui::PopID();
        }
    }

    ImGui::End();
}

void draw_light_system() {
    ImGui::SetNextWindowPos(ImVec2(10, 320), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
    ImGui::Begin("Add Lights", nullptr, ImGuiWindowFlags_NoResize);

    ImGui::Combo("Light Type", &selectedLightType, lightTypes, IM_ARRAYSIZE(lightTypes));

    if (ImGui::Button("Add Light")) {
        Light newLight;
        newLight.id = generate_unique_id();
        newLight.type = lightTypes[selectedLightType];
        lights.push_back(newLight);
    }

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(1590, 620), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Always);
    ImGui::Begin("Light Hierarchy", nullptr, ImGuiWindowFlags_NoResize);

    for (size_t i = 0; i < lights.size(); ++i) {
        Light& light = lights[i];
        std::string headerLabel = light.type + " Light (ID: " + std::to_string(light.id) + ")";
        if (ImGui::CollapsingHeader(headerLabel.c_str())) {
            ImGui::PushID(light.id);

            ImGui::DragFloat3("Position", glm::value_ptr(light.position), 0.1f);
            if (light.type == "Directional") {
                ImGui::DragFloat3("Rotation", glm::value_ptr(light.rotation), 0.1f);
            }
            ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
            ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 10.0f);

            if (ImGui::Button("Delete")) {
                lights.erase(lights.begin() + i);
                ImGui::PopID();
                break;
            }

            ImGui::PopID();
        }
    }

    ImGui::End();
}
glm::vec3 calculateLookAt() {
    glm::vec3 forward = glm::normalize(cameraForward); // Use camera forward direction
    return camera_position + forward; // Calculate look-at position relative to the camera
}

void processFirstPersonMovement(GLFWwindow* window) {
    if (firstPersonMode) {
        float movementSpeed = 0.05f; 

        // Dynamically calculate lookatPosition
        glm::vec3 lookatPosition = calculateLookAt();

        // Calculate forward vector (normalized direction from camera to lookat)
        glm::vec3 forwardVector = glm::normalize(lookatPosition - camera_position);

        // Calculate left vector (cross product of up and forward)
        glm::vec3 leftVector = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), forwardVector));

        // Moving forward/backward (W/S)
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camera_position += forwardVector * movementSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camera_position -= forwardVector * movementSpeed;
        }

        // Moving left/right (A/D)
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            camera_position += leftVector * movementSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camera_position -= leftVector * movementSpeed;
        }

        // Moving up/down (Q/E)
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            camera_position.y += movementSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            camera_position.y -= movementSpeed;
        }
        // Exit first-person mode
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            ExitFSMode();
        }
    }
}



double lastX = 0.0, lastY = 0.0;  // Store last mouse position

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstPersonMode) {
        float sensitivity = 0.01f;

        // Calculate delta movement from the previous mouse position
        float deltaX = xpos - lastX;
        float deltaY = ypos - lastY;

        // Update camera rotation based on the mouse movement
        camera_rotation.y -= deltaX * sensitivity; 
        camera_rotation.x -= deltaY * sensitivity; 

        // Clamp pitch to avoid flipping the camera
        if (camera_rotation.x > 15.0f) camera_rotation.x = 15.0f;
        if (camera_rotation.x < -15.0f) camera_rotation.x = -15.0f;

        lastX = xpos;
        lastY = ypos;
    }
}



void draw_gui(GLFWwindow* window) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::SetWindowFontScale(1.2);
    if (!firstPersonMode) {
        draw_mesh_import_window();
        draw_hierarchy_window();
        draw_light_system();
        drawCubeMapWindow();
    }
    drawCameraOptions();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void render_light_indicators(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) {
    glUseProgram(shaderProgram); // Use the same shader

    for (const auto& light : lights) {
        glm::mat4 T = glm::translate(light.position); // Position the light indicator
        glm::mat4 S = glm::scale(glm::vec3(0.1f));    // Scale it down
        glm::mat4 M = T * S;
        glm::mat4 PVM = projectionMatrix * viewMatrix * M;

        // Pass transformation matrix
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "PVM"), 1, GL_FALSE, glm::value_ptr(PVM));

        // Pass light color as the indicator color
        glUniform4fv(glGetUniformLocation(shaderProgram, "tintColor"), 1, glm::value_ptr(glm::vec4(light.color, 1.0f)));

        // Render a single point at the light position
        glPointSize(10.0f);  // Customize the point size
        glDrawArrays(GL_POINTS, 0, 1);
    }
}



void display(GLFWwindow* window) {
    if (clearScreen) {
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    glm::mat4 V = getViewMatrix();
    glm::mat4 P = getProjectionMatrix(aspect_ratio);
    initSkyBoxShader();
    renderSkybox();
    render_light_indicators(P, V);


    for (auto& mesh : importedMeshes) {
        glm::mat4 T = glm::translate(mesh.position);
        glm::mat4 S = glm::scale(mesh.scale);
        glm::mat4 R = glm::rotate(mesh.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) *
            glm::rotate(mesh.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(mesh.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 M = T * S * R;
        glm::mat4 PVM = P * V * M;
        if (mesh.hasTexture && mesh.textureID != -1) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "diffuse_tex"), 0);
        }
        else {
            glBindTexture(GL_TEXTURE_2D, 0); // Unbind if no texture
        }
        if (mesh.hasNormalMap && mesh.normalMapID != -1) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, mesh.normalMapID);
            glUniform1i(glGetUniformLocation(shaderProgram, "normal_tex"), 1);
        }
        else {
            glBindTexture(GL_TEXTURE_2D, 0); // Unbind if no normal map
        }


        glUseProgram(shaderProgram);

        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), mesh.hasTexture ? 1 : 0);
        if (mesh.hasTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "diffuse_tex"), 0);
        }
        glUniform1f(glGetUniformLocation(shaderProgram, "shininess"), mesh.shininess);
        glUniform4fv(glGetUniformLocation(shaderProgram, "tintColor"), 1, glm::value_ptr(mesh.tintColor));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "PVM"), 1, false, glm::value_ptr(PVM));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "M"), 1, false, glm::value_ptr(M));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(camera_position));


        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0); // Unbind any 3D textures
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glUniform1i(glGetUniformLocation(shaderProgram, "environmentMap"), 1);
        glUniform1f(glGetUniformLocation(shaderProgram, "hdrIntensity"), cubeMapIntensity);

        // Set HDR rotation (if you are applying rotation to the cubemap)
        glUniform3fv(glGetUniformLocation(shaderProgram, "hdrRotation"), 1, glm::value_ptr(cubemapRotation));


        glUniform1i(glGetUniformLocation(shaderProgram, "lightCount"), lights.size());
        for (size_t i = 0; i < lights.size(); ++i) {
            std::string prefix = "lightPos[" + std::to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(shaderProgram, prefix.c_str()), 1, glm::value_ptr(lights[i].position));

            prefix = "lightColor[" + std::to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(shaderProgram, prefix.c_str()), 1, glm::value_ptr(lights[i].color));

            prefix = "lightIntensity[" + std::to_string(i) + "]";
            glUniform1f(glGetUniformLocation(shaderProgram, prefix.c_str()), lights[i].intensity);

            prefix = "lightType[" + std::to_string(i) + "]";
            glUniform1i(glGetUniformLocation(shaderProgram, prefix.c_str()), lights[i].type == "Point" ? 0 : 1);
        }

        glBindVertexArray(mesh.mesh.mVao);
        mesh.mesh.DrawMesh();
    }
 
    processFirstPersonMovement(window);
    draw_gui(window);
    glfwSwapBuffers(window);
}

void reload_shader() {
    std::string vs = "shaders/f_vs.glsl";
    std::string fs = "shaders/f_fs.glsl";

    GLuint new_shader = InitShader(vs.c_str(), fs.c_str());
    if (new_shader != -1) {
        if (shaderProgram != -1) {
            glDeleteProgram(shaderProgram);
        }
        shaderProgram = new_shader;
        std::cout << "Shader Refreshed" << std::endl;
    }
}


void init() {
    glewInit();
    RegisterDebugCallback();


    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);  // Use counter-clockwise vertex order as the front face
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);

    reload_shader();
    initSkyBoxShader();
    initSkybox(cubemapPaths);
    glClearColor(1.0f, 0.35f, 0.35f, 1.0f);

}

int main(void) {
    GLFWwindow* window;

    if (!glfwInit()) {
        return -1;
    }

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(window_width, window_height, "OpenGL Scene Scroller", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    else {
        currentContext = window;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwMakeContextCurrent(window);

    init();
    glfwSetCursorPosCallback(window, mouseCallback);

   


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    while (!glfwWindowShouldClose(window)) {
        display(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
