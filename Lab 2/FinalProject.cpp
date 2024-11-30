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
GLuint skyboxVAO, skyboxVBO, cubemapTexture, skyboxShaderProgram;
float hdrIntensity;
std::string hdriFilePath = "";
glm::vec3 hdrRotation = glm::vec3(0.0f);


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
GLuint createDummyCubemap() {
    GLuint dummyCubemap;
    glGenTextures(1, &dummyCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, dummyCubemap);

    unsigned char black[3] = { 0, 0, 0 }; // Black color
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, black);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return dummyCubemap;
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

void initSkybox() {
    float skyboxVertices[] = {
        // Positions          
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


    // Generate VAO and VBO for the skybox
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glGenVertexArrays(1, &skyboxVAO);



    if (cubemapTexture == 0) {
        cubemapTexture = createDummyCubemap();
    }
    

    std::string vs = "shaders/skybox_vs.glsl";
    std::string fs = "shaders/skybox_fs.glsl";
    std::cout << "Vertex Shader Source:\n" << vs << std::endl;
    std::cout << "Fragment Shader Source:\n" << fs << std::endl;
    glUseProgram(skyboxShaderProgram);
    testFileRead(fs);
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    if (currentProgram != skyboxShaderProgram) {
        std::cerr << "Skybox shader program is not active!" << std::endl;
    }

    GLuint skyboxShaderProgram = InitShader(vs.c_str(), fs.c_str());
}


GLuint loadHDRCubemap(const std::string& filepath) {
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* data = stbi_loadf(filepath.c_str(), &width, &height, &nrComponents, 0);
    if (!data) {
        std::cerr << "Failed to load HDR image: " << filepath << std::endl;
        return 0;
    }

    GLuint hdrCubemap;
    glGenTextures(1, &hdrCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, hdrCubemap);

    // Allocate cubemap faces (you'll need to fill them via rendering)
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    return hdrCubemap;
}

void renderSkybox(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) {
    glDepthFunc(GL_LEQUAL); // Ensure skybox is rendered behind everything
    glUseProgram(skyboxShaderProgram);

    // Remove translation from the view matrix
    glm::mat4 view = glm::mat4(glm::mat3(viewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniform1f(glGetUniformLocation(skyboxShaderProgram, "hdrIntensity"), hdrIntensity);
    glBindVertexArray(skyboxVAO);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDepthFunc(GL_LESS); // Reset depth function
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


void drawHDRIWindow() {
    ImGui::SetNextWindowPos(ImVec2(10, 540), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
    ImGui::Begin("HDRI Management", nullptr, ImGuiWindowFlags_NoResize);
    IGFD::FileDialogConfig fileDialogConfig;
    fileDialogConfig.path = ".";
    fileDialogConfig.fileName = "";
    fileDialogConfig.countSelectionMax = 1;
    fileDialogConfig.flags = ImGuiFileDialogFlags_None;

    if (hdriFilePath.empty()) {
        if (ImGui::Button("Import HDRI")) {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseHDRIFile", "Select HDRI", ".hdr,.exr", fileDialogConfig);
        }
    }
    else {
        ImGui::Text("HDRI Loaded: %s", hdriFilePath.c_str());
        ImGui::SliderFloat("HDR Intensity", &hdrIntensity, 0.1f, 10.0f);
        ImGui::DragFloat3("HDR Rotation", glm::value_ptr(hdrRotation), 0.1f);
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseHDRIFile", 32, ImVec2(500, 500), ImVec2(800, 800))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            hdriFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
            cubemapTexture = loadHDRCubemap(hdriFilePath);
        }
        ImGuiFileDialog::Instance()->Close();
    }

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
            ImGui::DragFloat("Shininess", &mesh.shininess, 1.0f, 1.0f, 128.0f);

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

void draw_gui(GLFWwindow* window) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::SetWindowFontScale(1.2);
    draw_mesh_import_window();
    draw_hierarchy_window();
    draw_light_system();
    drawHDRIWindow();

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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    glm::mat4 V = glm::lookAt(camera_position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 P = glm::perspective(camera_fov, aspect_ratio, near_clip, far_clip);

   renderSkybox(P, V);

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
    render_light_indicators(P, V);

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
    initSkybox();
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
