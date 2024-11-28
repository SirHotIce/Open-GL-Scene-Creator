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

bool clearScreen = true;
bool enableDepthTest = true;
glm::vec3 camera_position = glm::vec3(0.0f, 0.0f, 1.0f);

float camera_fov = glm::radians(45.0f);
float near_clip = 0.1f;
float far_clip = 100.0f;

int window_width = 2000;
int window_height = 1200;
float aspect_ratio = 1.0f;

GLuint shaderProgram = -1;

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
    glm::vec3 rotation = glm::vec3(0.0f); // Roll, Pitch, Yaw
    glm::vec3 scale = glm::vec3(1.0f);
    float shininess = 32.0f; // Default shininess
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

struct Light {
    int id; // Unique identifier
    std::string type; // "Point" or "Directional"
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); // For directional light
    glm::vec3 color = glm::vec3(1.0f); // RGB
    float intensity = 1.0f;
};

std::vector<Light> lights;
int selectedLightType = 0; // 0 = Point, 1 = Directional
const char* lightTypes[] = { "Point", "Directional" };


int generate_unique_id() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, 1000000);
    return dist(rng);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    window_width = width;
    window_height = height;
    glViewport(0, 0, width, height);
    aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
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
        ImGuiFileDialog::Instance()->OpenDialog("ChooseMeshFile", "Select Mesh File", ".obj,.fbx,.dae", fileDialogConfig);
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

void draw_hierarchy_window() {
    ImGui::SetNextWindowPos(ImVec2(1590, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_Always);
    ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoResize);

    for (size_t i = 0; i < importedMeshes.size(); ++i) {
        ImportedMesh& mesh = importedMeshes[i];
        std::string headerLabel = mesh.name + " (ID: " + std::to_string(mesh.id) + ")";
        if (ImGui::CollapsingHeader(headerLabel.c_str())) {
            ImGui::PushID(mesh.id);

            ImGui::DragFloat3("Position", glm::value_ptr(mesh.position), 0.1f);
            ImGui::DragFloat3("Rotation (Roll, Pitch, Yaw)", glm::value_ptr(mesh.rotation), 0.1f, -6.28, 6.28);
            ImGui::DragFloat3("Scale", glm::value_ptr(mesh.scale), 0.1f);
            ImGui::ColorEdit4("Tint", glm::value_ptr(mesh.tintColor));
            ImGui::DragFloat("Shininess", &mesh.shininess, 1.0f, 1.0f, 128.0f);

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

void draw_gui(GLFWwindow* window) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::SetWindowFontScale(1.2);
    draw_mesh_import_window();
    draw_hierarchy_window();
    draw_light_system();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void display(GLFWwindow* window) {
    if (clearScreen) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    glm::mat4 V = glm::lookAt(camera_position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 P = glm::perspective(camera_fov, aspect_ratio, near_clip, far_clip);

    for (auto& mesh : importedMeshes) {
        glm::mat4 T = glm::translate(mesh.position);
        glm::mat4 S = glm::scale(mesh.scale);
        glm::mat4 R = glm::rotate(mesh.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) *
            glm::rotate(mesh.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(mesh.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 M = T * S * R;
        glm::mat4 PVM = P * V * M;

        glUseProgram(shaderProgram);

        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), mesh.hasTexture ? 1 : 0);
        if (mesh.hasTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "diffuse_tex"), 0);
        }

        glUniform4fv(glGetUniformLocation(shaderProgram, "tintColor"), 1, glm::value_ptr(mesh.tintColor));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "PVM"), 1, false, glm::value_ptr(PVM));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "M"), 1, false, glm::value_ptr(M));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(camera_position));

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
    }
}

void init() {
    glewInit();
    RegisterDebugCallback();

    glClearColor(0.35f, 0.35f, 0.35f, 1.0f);
    if (enableDepthTest)
        glEnable(GL_DEPTH_TEST);

    reload_shader();
}

int main(void) {
    GLFWwindow* window;

    if (!glfwInit()) {
        return -1;
    }

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    window = glfwCreateWindow(window_width, window_height, "OpenGL Scene Scroller", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwMakeContextCurrent(window);
    init();

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
