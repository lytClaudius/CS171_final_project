#include <camera.h>
#include <shader.h>
#include <utils.h>
#include <iostream>
#include <vector>

const int numCascades = 4;
const int WIDTH = 1920;
const int HEIGHT = 1080;
GLuint sdfFBO, sdfTex;
GLuint cascadeFBOs[numCascades], cascadeTexs[numCascades];
GLuint mergeFBOs[numCascades], mergeTexs[numCascades];
GLuint radianceFieldFBO, radianceFieldTex;

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
    // 1. 创建 SDF 纹理 (RGBA32F)
    glGenFramebuffers(1, &sdfFBO);
    glGenTextures(1, &sdfTex);
    glBindTexture(GL_TEXTURE_2D, sdfTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    setTexParams(); // <--- 加上这个
    // ... 设置过滤参数 ...
    glBindFramebuffer(GL_FRAMEBUFFER, sdfFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sdfTex, 0);

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

// ... 之前的全局变量保持不变 ...

int main()
{
    // 1. GLFW/GLAD 初始化
    glfwInit();
    // ... windowGuard 逻辑 ...
    WindowGuard windowGuard(window, WIDTH, HEIGHT, "SDF Cascaded Rendering");

    // 2. 加载 Shader
    Shader sdfShader("shaders/basic.vs", {"shaders/common.glsl", "shaders/sdf_texture.fs"});
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

    while (!glfwWindowShouldClose(window))
    {
        // --- 开始渲染帧 ---
        glViewport(0, 0, WIDTH, HEIGHT);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        // --- STEP 1: 生成场景 SDF ---
        glBindFramebuffer(GL_FRAMEBUFFER, sdfFBO);
        sdfShader.use();
        sdfShader.setVec2("resolution", glm::vec2(WIDTH, HEIGHT));
        drawQuad();

        // --- STEP 2: 计算 Radiance ---
        computeShader.use();
        computeShader.setVec2("cascadeResolution", glm::vec2(WIDTH, HEIGHT));
        computeShader.setVec2("sceneResolution", glm::vec2(WIDTH, HEIGHT));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sdfTex);
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

        // 槽位 0: 绑定刚刚算好的平滑光照
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, radianceFieldTex);

        // 槽位 1: 绑定 SDF 纹理用于绘制物体颜色
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, sdfTex);
        drawQuad();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}