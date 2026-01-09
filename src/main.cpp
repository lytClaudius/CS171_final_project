#include <camera.h>
#include <shader.h>
#include <utils.h>
#include <iostream>
#include <vector>

bool firstMouse = true;
const int numCascades = 4;
const int WIDTH = 1920;
const int HEIGHT = 1080;
float lastX = WIDTH / 2.0;
float lastY = HEIGHT / 2.0;
GLuint sdfFBO[2], sdfTex[2], tmpSdfFBO, tmpSdfTex; // Ping-pong FBO
GLuint cascadeFBOs[numCascades], cascadeTexs[numCascades];
GLuint mergeFBOs[numCascades], mergeTexs[numCascades];
GLuint radianceFieldFBO, radianceFieldTex;
GLuint brushAccumFBO, brushAccumTex;
int sdfReadIdx = 0;

float quadVertices[] = {
    -1.0f, 1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
    1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 1.0f};

GLFWwindow *window;
GLuint blackTex;
GLuint quadVAO = 0;

// Brush state
glm::vec2 brushPos = glm::vec2(0, 0);
glm::vec2 prevBrushPos = brushPos;
float brushRadius = 20.0f;
glm::vec3 brushColour = glm::vec3(1.0f, 0.0f, 0.0f);
bool isDrawing = false;

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

void setTexParams()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}
void initBlackTex()
{
    glGenTextures(1, &blackTex);
    glBindTexture(GL_TEXTURE_2D, blackTex);
    float black[] = {0, 0, 0, 0};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, black);
    setTexParams();
}

void initResources()
{
    for (int i = 0; i < 2; i++)
    {
        glGenFramebuffers(1, &sdfFBO[i]);
        glGenTextures(1, &sdfTex[i]);
        glBindTexture(GL_TEXTURE_2D, sdfTex[i]);
        // 使用 RGBA32F 保证精度
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        setTexParams();
        glBindFramebuffer(GL_FRAMEBUFFER, sdfFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sdfTex[i], 0);

        // 【关键】初始化清空画布：颜色为黑，距离(Alpha)为很大(10000)
        float clearColor[] = {0.0f, 0.0f, 0.0f, 10000.0f};
        glClearBufferfv(GL_COLOR, 0, clearColor);
    }

    glGenFramebuffers(1, &tmpSdfFBO);
    glGenTextures(1, &tmpSdfTex);
    glBindTexture(GL_TEXTURE_2D, tmpSdfTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    setTexParams();
    glBindFramebuffer(GL_FRAMEBUFFER, tmpSdfFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tmpSdfTex, 0);

    // 2. 为每一级级联创建 FBO 和 纹理
    for (int i = 0; i < numCascades; i++)
    {
        // Cascade 原始检测结果纹理
        glGenFramebuffers(1, &cascadeFBOs[i]);
        glGenTextures(1, &cascadeTexs[i]);
        glBindTexture(GL_TEXTURE_2D, cascadeTexs[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        setTexParams();
        glBindFramebuffer(GL_FRAMEBUFFER, cascadeFBOs[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cascadeTexs[i], 0);

        // Merge 合并结果纹理
        glGenFramebuffers(1, &mergeFBOs[i]);
        glGenTextures(1, &mergeTexs[i]);
        glBindTexture(GL_TEXTURE_2D, mergeTexs[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        setTexParams();
        glBindFramebuffer(GL_FRAMEBUFFER, mergeFBOs[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mergeTexs[i], 0);
    }

    glGenFramebuffers(1, &radianceFieldFBO);
    glGenTextures(1, &radianceFieldTex);
    glBindTexture(GL_TEXTURE_2D, radianceFieldTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    setTexParams();
    glBindFramebuffer(GL_FRAMEBUFFER, radianceFieldFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, radianceFieldTex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Radiance Field FBO not complete!" << std::endl;
}

// Mouse input callback
void processInput(GLFWwindow *window)
{
    return;
}

void mouse_callback(GLFWwindow *window, double x, double y)
{
    x = (float)x;
    y = (float)y;
    if (firstMouse)
    {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }
    // main.cpp mouse_callback
    brushPos = glm::vec2((float)x / WIDTH, (float)(HEIGHT - y) / HEIGHT);
    lastX = x;
    lastY = y;
}

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            isDrawing = true;
            prevBrushPos = brushPos;
        }
        else if (action == GLFW_RELEASE)
        {
            isDrawing = false;
        }
    }
}

// ... 之前的全局变量保持不变 ...

int main()
{
    // 1. GLFW/GLAD 初始化
    glfwInit();
    // ... windowGuard 逻辑 ...
    WindowGuard windowGuard(window, WIDTH, HEIGHT, "SDF Cascaded Rendering");

    // Set up mouse and keyboard callbacks
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // 2. 加载 Shader
    Shader sdfShader("shaders/basic.vs", {"shaders/common.glsl", "shaders/scene-fragment-shader.glsl"});
    Shader brushShader("shaders/basic.vs", {"shaders/common.glsl", "shaders/scene-accumulation-fragment-shader.glsl"});
    Shader computeShader("shaders/basic.vs", {"shaders/common.glsl", "shaders/cascades.glsl", "shaders/compute-cascade-fragment-shader.glsl"});
    Shader mergeShader("shaders/basic.vs", {"shaders/common.glsl", "shaders/cascades.glsl", "shaders/merge-cascades-fragment-shader.glsl"});
    Shader finalShader("shaders/basic.vs", {"shaders/common.glsl", "shaders/final-compose-fragment-shader.glsl"});
    Shader radianceShader("shaders/basic.vs", {"shaders/common.glsl", "shaders/cascades.glsl", "shaders/radiance-field-fragment-shader.glsl"});

    // 3. !!! 重要：初始化资源 !!!
    initResources();
    initBlackTex();

    // 4. 设置静态 Uniform
    float cascade0_Range = 16.0f;
    int cascade0_Dims = 6;

    computeShader.use();
    computeShader.setFloat("cascade0_Range", cascade0_Range);
    computeShader.setInt("cascade0_Dims", cascade0_Dims);
    computeShader.setInt("sdfTexture", 0); // 确保与 FS 里的变量名对应

    mergeShader.use();
    mergeShader.setFloat("cascade0_Range", cascade0_Range);
    mergeShader.setInt("cascade0_Dims", cascade0_Dims);
    mergeShader.setInt("cascadeRealTexture", 0);
    mergeShader.setInt("nextCascadeMergedTexture", 1);

    radianceShader.use();
    radianceShader.setFloat("cascade0_Range", cascade0_Range);
    radianceShader.setInt("cascade0_Dims", cascade0_Dims);
    radianceShader.setInt("mergedCascade0Texture", 0); // 采样槽位 0
    radianceShader.setVec2("cascadeResolution", glm::vec2(WIDTH, HEIGHT));

    finalShader.use();
    finalShader.setInt("radianceTexture", 0); // 改为读取积分后的纹理
    finalShader.setInt("sdfTexture", 1);      // 读取场景/SDF 纹理用于混合物体
    finalShader.setVec2("resolution", glm::vec2(WIDTH, HEIGHT));
    finalShader.setFloat("brushRadius", brushRadius);
    finalShader.setVec3("brushColour", brushColour);

    while (!glfwWindowShouldClose(window))
    {
        // --- 开始渲染帧 ---
        glViewport(0, 0, WIDTH, HEIGHT);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        int sdfWriteIdx = 1 - sdfReadIdx;
        glBindFramebuffer(GL_FRAMEBUFFER, sdfFBO[sdfWriteIdx]);
        brushShader.use();
        brushShader.setVec2("resolution", glm::vec2(WIDTH, HEIGHT));
        brushShader.setVec2("brushPos1", prevBrushPos);
        brushShader.setVec2("brushPos2", brushPos);
        brushShader.setFloat("brushRadius", brushRadius);
        brushShader.setVec3("brushColour", brushColour);
        brushShader.setBool("isDrawing", isDrawing);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sdfTex[sdfReadIdx]);
        brushShader.setInt("sdfTexture", 0);
        drawQuad();
        sdfReadIdx = sdfWriteIdx;

        // --- STEP 1: 生成场景 SDF ---
        glBindFramebuffer(GL_FRAMEBUFFER, tmpSdfFBO);
        sdfShader.use();
        sdfShader.setVec2("resolution", glm::vec2(WIDTH, HEIGHT));
        sdfShader.setVec2("brushPos", brushPos);
        sdfShader.setFloat("brushRadius", brushRadius);
        sdfShader.setVec3("brushColour", brushColour);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sdfTex[sdfReadIdx]);
        sdfShader.setInt("sdfTexture", 0);
        drawQuad();

        // --- STEP 2: 计算 Radiance ---
        computeShader.use();
        computeShader.setVec2("cascadeResolution", glm::vec2(WIDTH, HEIGHT));
        computeShader.setVec2("sceneResolution", glm::vec2(WIDTH, HEIGHT));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tmpSdfTex); // 使用刚刚生成的 SDF 纹理
        for (int i = 0; i < numCascades; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, cascadeFBOs[i]);
            computeShader.setInt("cascadeLevel", i);
            drawQuad();
        }

        // --- STEP 3: 逆序合并 ---
        mergeShader.use();
        mergeShader.setVec2("cascadeResolution", glm::vec2(WIDTH, HEIGHT));
        mergeShader.setVec2("sceneResolution", glm::vec2(WIDTH, HEIGHT));
        mergeShader.setInt("numCascadeLevels", numCascades);
        for (int i = numCascades - 1; i >= 0; i--)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, mergeFBOs[i]);
            mergeShader.setInt("currentCascadeLevel", i);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cascadeTexs[i]);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, (i == numCascades - 1) ? blackTex : mergeTexs[i + 1]);

            drawQuad();
        }

        // --- STEP 4: 角度积分 (将 L0 原始数据转为平滑光照) ---
        glBindFramebuffer(GL_FRAMEBUFFER, radianceFieldFBO);
        radianceShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mergeTexs[0]); // 输入：Merge Pass 的结果
        drawQuad();

        // --- STEP 5: 最终合成显示 (Final Pass) ---
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // 回到屏幕
        glClear(GL_COLOR_BUFFER_BIT);
        finalShader.use();

        // Update brush cursor position for visualization
        finalShader.setVec2("brushPos", brushPos);
        finalShader.setFloat("brushRadius", brushRadius);
        finalShader.setVec3("brushColour", brushColour);

        // 槽位 0: 绑定刚刚算好的平滑光照
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, radianceFieldTex);

        // 槽位 1: 绑定 SDF 纹理用于绘制物体颜色
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tmpSdfTex);
        drawQuad();
        prevBrushPos = brushPos;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}