#include <camera.h>
#include <shader.h>
#include <utils.h>
#include <iostream>
#include <vector>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
bool firstMouse = true;
bool cameraIsRotating = false;
float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;

// 光照图集 Atlas 的分辨率
const int ATLAS_WIDTH = 1024;
const int ATLAS_HEIGHT = 6144; // 1024 * 6

// Ping-Pong 资源
GLuint atlasFBO[2];
GLuint atlasTex[2];
int atlasReadIdx = 0;
int atlasWriteIdx = 1;

GLuint quadVAO = 0;
float quadVertices[] = {
    -1.0f, 1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
    1.0f, -1.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 1.0f};

GLFWwindow *window;
Camera camera;

void drawQuad()
{
    if (quadVAO == 0)
    {
        GLuint vbo;
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &vbo);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void initResources()
{
    // 初始化两个 FBO 和 纹理用于 Ping-Pong
    for (int i = 0; i < 2; i++)
    {
        glGenFramebuffers(1, &atlasFBO[i]);
        glGenTextures(1, &atlasTex[i]);

        glBindTexture(GL_TEXTURE_2D, atlasTex[i]);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, ATLAS_WIDTH, ATLAS_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        std::vector<float> emptyData(ATLAS_WIDTH * ATLAS_HEIGHT * 4, 0.0f);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ATLAS_WIDTH, ATLAS_HEIGHT, GL_RGBA, GL_FLOAT, emptyData.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindFramebuffer(GL_FRAMEBUFFER, atlasFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, atlasTex[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Atlas FBO[" << i << "] not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        camera.processWalkAround(Forward);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        camera.processWalkAround(Backward);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        camera.processWalkAround(Leftward);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        camera.processWalkAround(Rightward);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        camera.processWalkAround(Upward);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        camera.processWalkAround(Downward);
    }
}

void mouse_callback(GLFWwindow *window, double x, double y)
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouse = true;
        return;
    }

    if (!cameraIsRotating)
        return;
    x = (float)x;
    y = (float)y;
    if (firstMouse)
    {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }

    camera.processLookAround(x - lastX, y - lastY);

    // update record
    lastX = x;
    lastY = y;
}

void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            cameraIsRotating = true;
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            lastX = xpos;
            lastY = ypos;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else if (action == GLFW_RELEASE)
        {
            cameraIsRotating = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "3D Radiance Cascades (OpenGL)", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // 设置回调
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // shaders
    Shader bakingShader("3Dshaders/basic3D.vs", {"3Dshaders/common.glsl", "3Dshaders/radiance-field-3d-fragment-shader.glsl"});
    Shader displayShader("3Dshaders/basic3D.vs", {"3Dshaders/common.glsl", "3Dshaders/final-3d-display.fs"});

    initResources();

    // ImGui 初始化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::StyleColorsDark();

    // 设置 Shader 的固定 Uniform (纹理槽位)
    bakingShader.use();
    bakingShader.setInt("iChannel3", 0);
    bakingShader.setVec2("iResolution", glm::vec2(ATLAS_WIDTH, ATLAS_HEIGHT)); // Atlas 尺寸

    displayShader.use();
    displayShader.setInt("radianceAtlas", 0);
    displayShader.setVec2("iResolution", glm::vec2(WINDOW_WIDTH, WINDOW_HEIGHT)); // 屏幕尺寸

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        float currentTime = (float)glfwGetTime();

        // -----------------------------------------------------
        // Pass 1: Radiance Baking (烘焙光照到 Atlas)
        // -----------------------------------------------------
        // 绑定写入 FBO
        glBindFramebuffer(GL_FRAMEBUFFER, atlasFBO[atlasWriteIdx]);
        glViewport(0, 0, ATLAS_WIDTH, ATLAS_HEIGHT);
        bakingShader.use();
        bakingShader.setFloat("iTime", currentTime);

        // 绑定上一帧的纹理到槽位 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlasTex[atlasReadIdx]);

        drawQuad();

        // 因为 TraceRay 和 Sample 逻辑中可能依赖 textureLod
        glBindTexture(GL_TEXTURE_2D, atlasTex[atlasWriteIdx]);
        glGenerateMipmap(GL_TEXTURE_2D);

        // -----------------------------------------------------
        // Pass 2: Final Display (渲染到屏幕)
        // -----------------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        displayShader.use();
        displayShader.setFloat("iTime", currentTime);
        displayShader.setVec3("uCamPos", camera.Position);
        displayShader.setVec3("uCamFront", camera.Front);

        // 绑定刚刚写入的 Atlas 纹理供显示 Shader 采样
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlasTex[atlasWriteIdx]);

        drawQuad();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Debug Info");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Time: %.2f", currentTime);
        ImGui::Text("Resolution: %d x %d", WINDOW_WIDTH, WINDOW_HEIGHT);
        ImGui::Text("Atlas Size: %d x %d", ATLAS_WIDTH, ATLAS_HEIGHT);
        if (ImGui::Button("Reset Texture"))
        {
            float black[] = {0, 0, 0, 0};
            for (int i = 0; i < 2; i++)
            {
                glBindTexture(GL_TEXTURE_2D, atlasTex[i]);
                glClearTexImage(atlasTex[i], 0, GL_RGBA, GL_FLOAT, black);
            }
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // 交换 Ping-Pong 索引
        int temp = atlasReadIdx;
        atlasReadIdx = atlasWriteIdx;
        atlasWriteIdx = temp;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}