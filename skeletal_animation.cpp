#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>

#include <stb_image.h>
#include <iostream>
#include <algorithm>
#include <cmath>

// ---------- forward decl ----------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// ---------- settings ----------
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// ---------- camera (free cam object still exists) ----------
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// ---------- follow camera options ----------
bool  followCam = true;   // กด C สลับ Follow / Free
float camDistance = 4.0f;   // ระยะกล้องจากตัวละคร (ด้านหลัง)
float camHeight = 1.2f;   // ยกกล้องขึ้นจากพื้น
float camTargetHeight = 1.0f;   // ยกเป้าหมายมอง (หัวอก)

// ---------- timing ----------
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ---------- character ----------
glm::vec3 characterPos(0.0f, 0.0f, 0.0f);
float characterRotation = 180.0f; // เริ่มหันหลังให้กล้อง
float currentSpeed = 0.0f;

const float walkSpeed = 2.0f;
const float runSpeed = 5.0f;
const float acceleration = 15.0f;
const float deceleration = 10.0f;
const float turnSpeed = 120.0f;

// ---------- animations ----------
Animation* idleAnim = nullptr;       // Idle.dae
Animation* walkAnim = nullptr;       // walk.dae
Animation* runAnim = nullptr;        // run.dae
Animation* backwardAnim = nullptr;   // backward.dae
Animation* punchAnim = nullptr;
Animation* kickAnim = nullptr;
Animation* jumpAnim = nullptr;       // <-- เพิ่ม jump.dae

Animator* animator = nullptr;
Animation* currentAnim = nullptr;

// states
bool isPunching = false;
bool isKicking = false;
bool isJumping = false;              // <-- เพิ่มสถานะกระโดด one-shot
float jumpElapsed = 0.0f;
float jumpDuration = 0.0f;           // seconds

enum class Loco { Idle, Walk, Run, Backward };
Loco locoState = Loco::Idle;

// ---------- helper (force-switch animation by recreating Animator) ----------
void SwitchAnimation(Animation* next) {
    if (!next) return;
    if (currentAnim == next) return;
    if (animator) { delete animator; animator = nullptr; }
    animator = new Animator(next);
    currentAnim = next;
}

// ใช้ปุ่มตรง ๆ: Shift+W → Run, W → Walk, S → Backward, else Idle
void ApplyLocomotion(GLFWwindow* window, bool movingFwd, bool movingBack) {
    if (isPunching || isKicking || isJumping) return; // <-- กันระหว่างกระโดด
    bool shiftHeld = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
    if (movingFwd && shiftHeld) { if (locoState != Loco::Run) { SwitchAnimation(runAnim); locoState = Loco::Run; } return; }
    if (movingFwd) { if (locoState != Loco::Walk) { SwitchAnimation(walkAnim); locoState = Loco::Walk; } return; }
    if (movingBack) { if (locoState != Loco::Backward) { SwitchAnimation(backwardAnim); locoState = Loco::Backward; } return; }
    if (locoState != Loco::Idle) { SwitchAnimation(idleAnim); locoState = Loco::Idle; }
}

// scale เวลาอนิเมชันให้สัมพันธ์กับสปีดโลก
float CalcAnimScale() {
    float s = std::abs(currentSpeed);
    if (s < 0.05f) return 1.0f;
    float base = (locoState == Loco::Run) ? runSpeed : walkSpeed;
    if (locoState == Loco::Backward) base = walkSpeed;
    return std::clamp(s / std::max(base, 0.0001f), 0.7f, 1.4f);
}

// ---------- NEW: trigger jump one-shot ----------
void TriggerJumpOnce() {
    if (isJumping || isPunching || isKicking || !jumpAnim) return;
    isJumping = true;
    jumpElapsed = 0.0f;

    // คำนวณความยาวอนิเมชันจริงเป็นวินาทีจาก ticks
    float durTicks = static_cast<float>(jumpAnim->GetDuration());
    float tps = std::max<float>(1.0f, static_cast<float>(jumpAnim->GetTicksPerSecond()));
    jumpDuration = (tps > 0.0f) ? (durTicks / tps) : 0.8f; // fallback ราวๆ 0.8s

    SwitchAnimation(jumpAnim); // เล่น jump.dae ทันที
}

// ---------- global for background/ground ----------
unsigned int quadVAO = 0, quadVBO = 0;
unsigned int planeVAO = 0, planeVBO = 0;

// setup fullscreen quad
void SetupFullscreenQuad() {
    float quadVertices[] = {
        // pos      // uv
        -1.f,  1.f,  0.f, 1.f,
        -1.f, -1.f,  0.f, 0.f,
         1.f, -1.f,  1.f, 0.f,
        -1.f,  1.f,  0.f, 1.f,
         1.f, -1.f,  1.f, 0.f,
         1.f,  1.f,  1.f, 1.f
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

// setup big plane for ground (positions, normals, uvs)
void SetupGroundPlane() {
    float planeVertices[] = {
        // pos                // normal        // uv
        -1.0f, 0.0f, -1.0f,    0,1,0,          0.0f, 0.0f,
         1.0f, 0.0f, -1.0f,    0,1,0,          1.0f, 0.0f,
         1.0f, 0.0f,  1.0f,    0,1,0,          1.0f, 1.0f,

        -1.0f, 0.0f, -1.0f,    0,1,0,          0.0f, 0.0f,
         1.0f, 0.0f,  1.0f,    0,1,0,          1.0f, 1.0f,
        -1.0f, 0.0f,  1.0f,    0,1,0,          0.0f, 1.0f,
    };
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);
}

int main()
{
    // ---------- window / GL init ----------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hero Controller", nullptr, nullptr);
    if (!window) { std::cout << "Failed to create window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cout << "Failed to init GLAD\n"; return -1; }

    stbi_set_flip_vertically_on_load(true);
    glEnable(GL_DEPTH_TEST);

    // ---------- shaders ----------
    Shader ourShader("anim_model.vs", "anim_model.fs");     // ของเดิม
    Shader bgShader("background.vs", "background.fs");      // พื้นหลังไล่สี
    Shader groundShader("ground.vs", "ground.fs");          // พื้น + กริด + หมอก

    // ---------- model ----------
    std::cout << "Loading hero model...\n";
    Model heroModel(FileSystem::getPath("resources/objects/hero/hero.dae"));
    std::cout << "Hero model loaded.\n";

    // ---------- animations ----------
    std::cout << "Loading animations...\n";
    idleAnim = new Animation(FileSystem::getPath("resources/objects/hero/Idle.dae"), &heroModel);
    walkAnim = new Animation(FileSystem::getPath("resources/objects/hero/walk.dae"), &heroModel);
    runAnim = new Animation(FileSystem::getPath("resources/objects/hero/run.dae"), &heroModel);
    backwardAnim = new Animation(FileSystem::getPath("resources/objects/hero/backward.dae"), &heroModel);
    punchAnim = new Animation(FileSystem::getPath("resources/objects/hero/punch.dae"), &heroModel);
    kickAnim = new Animation(FileSystem::getPath("resources/objects/hero/kick.dae"), &heroModel);
    jumpAnim = new Animation(FileSystem::getPath("resources/objects/hero/jump.dae"), &heroModel); // <-- โหลด jump
    std::cout << "Animations loaded.\n";

    animator = new Animator(idleAnim);
    currentAnim = idleAnim;
    locoState = Loco::Idle;

    // ---------- geometry for background & ground ----------
    SetupFullscreenQuad();
    SetupGroundPlane();

    std::cout << "\nControls:\n"
        << "W - Walk forward, Shift+W - Run forward\n"
        << "S - Walk backward\n"
        << "A/D - Turn left/right\n"
        << "SPACE - Jump (one-shot)\n"
        << "C - Toggle FollowCam / FreeCam\n"
        << "Wheel - Zoom (when FollowCam)\n"
        << "Q/E - Punch/Kick\n\n";

    while (!glfwWindowShouldClose(window))
    {
        // ---------- timing ----------
        float now = static_cast<float>(glfwGetTime());
        deltaTime = now - lastFrame;
        lastFrame = now;

        // ---------- input & movement ----------
        processInput(window);

        // อัปเดตตำแหน่งจาก currentSpeed
        if (std::abs(currentSpeed) > 1e-3f) {
            float r = glm::radians(characterRotation);
            characterPos.x += sin(r) * currentSpeed * deltaTime;
            characterPos.z += cos(r) * currentSpeed * deltaTime;
        }

        // เลือกอนิเมชันตามปุ่มกด (จะถูกกันไว้ถ้ากำลังกระโดด)
        bool movingFwd = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
        bool movingBack = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
        if (movingFwd && movingBack) movingBack = false; // ให้ W ชนะ
        ApplyLocomotion(window, movingFwd, movingBack);

        // อัปเดตอนิเมชัน
        float scale = CalcAnimScale();
        if (isJumping) scale = 1.0f; // jump เล่นด้วยสปีดคงที่
        animator->UpdateAnimation(deltaTime * scale);

        // ถ้าเป็น jump ให้จับเวลาจนจบ แล้วเด้งกลับ idle (เฟรมถัดไป locomotion จะเลือกต่อเอง)
        if (isJumping) {
            jumpElapsed += deltaTime;
            if (jumpElapsed >= jumpDuration - 1e-4f) {
                isJumping = false;
                SwitchAnimation(idleAnim);
            }
        }

        // ---------- compute camera matrices ----------
        glm::mat4 view;
        glm::vec3 camPosWS;
        if (followCam) {
            float r = glm::radians(characterRotation);
            glm::vec3 forward = glm::vec3(sin(r), 0.0f, cos(r));
            glm::vec3 target = characterPos + glm::vec3(0.0f, camTargetHeight, 0.0f);
            camPosWS = target - forward * camDistance + glm::vec3(0.0f, camHeight, 0.0f);
            view = glm::lookAt(camPosWS, target + forward * 0.001f, glm::vec3(0, 1, 0));
        }
        else {
            view = camera.GetViewMatrix();
            camPosWS = camera.Position;
        }

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        // ---------- render ----------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1) Background
        glDepthMask(GL_FALSE);
        bgShader.use();
        bgShader.setFloat("uTime", now);
        bgShader.setVec2("uResolution", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
        bgShader.setVec3("topColor", glm::vec3(0.09f, 0.11f, 0.22f));
        bgShader.setVec3("bottomColor", glm::vec3(0.55f, 0.62f, 0.84f));
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);

        // 2) Ground
        glEnable(GL_DEPTH_TEST);
        groundShader.use();
        groundShader.setMat4("projection", projection);
        groundShader.setMat4("view", view);

        glm::mat4 groundModel(1.0f);
        groundModel = glm::translate(groundModel, glm::vec3(0.0f, -0.4f, 0.0f));
        groundModel = glm::scale(groundModel, glm::vec3(200.0f, 1.0f, 200.0f));
        groundShader.setMat4("model", groundModel);

        groundShader.setVec3("uCamPos", camPosWS);
        groundShader.setVec3("uGridColor", glm::vec3(0.85f));
        groundShader.setVec3("uBaseColor", glm::vec3(0.78f));
        groundShader.setVec3("uHorizonColor", glm::vec3(0.62f, 0.68f, 0.88f));
        groundShader.setFloat("uGridScale", 2.0f);
        groundShader.setFloat("uLineWidth", 0.02f);
        groundShader.setFloat("uFogDensity", 0.006f);

        glBindVertexArray(planeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // 3) Character (skinned)
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        auto transforms = animator->GetFinalBoneMatrices();
        for (int i = 0; i < (int)transforms.size(); ++i)
            ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, characterPos);
        model = glm::rotate(model, glm::radians(characterRotation), glm::vec3(0, 1, 0));
        model = glm::translate(model, glm::vec3(0.0f, -0.4f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));
        ourShader.setMat4("model", model);

        heroModel.Draw(ourShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ---------- cleanup ----------
    delete idleAnim;
    delete walkAnim;
    delete runAnim;
    delete backwardAnim;
    delete punchAnim;
    delete kickAnim;
    delete jumpAnim;   // <-- cleanup
    delete animator;

    if (quadVBO) glDeleteBuffers(1, &quadVBO);
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (planeVBO) glDeleteBuffers(1, &planeVBO);
    if (planeVAO) glDeleteVertexArrays(1, &planeVAO);

    glfwTerminate();
    return 0;
}

// ---------- input ----------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Toggle Follow / Free
    static bool cHeld = false;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!cHeld) { followCam = !followCam; cHeld = true; }
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) cHeld = false;

    // กล้องอิสระ (ใช้เฉพาะตอน followCam = false)
    if (!followCam) {
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);
    }

    bool shiftHeld = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
    bool movingFwd = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
    bool movingBack = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
    if (movingFwd && movingBack) movingBack = false;

    // ความเร็วเป้าหมาย
    float targetSpeed = 0.0f;
    if (movingFwd && shiftHeld)       targetSpeed = runSpeed;
    else if (movingFwd)               targetSpeed = walkSpeed;
    else if (movingBack)              targetSpeed = -walkSpeed;

    // หมุนตัวละคร (A/D)
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) characterRotation += turnSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) characterRotation -= turnSpeed * deltaTime;

    // เร่ง/ผ่อน
    if (movingFwd || movingBack) {
        if (currentSpeed < targetSpeed)
            currentSpeed = std::min(currentSpeed + acceleration * deltaTime, targetSpeed);
        else if (currentSpeed > targetSpeed)
            currentSpeed = std::max(currentSpeed - acceleration * deltaTime, targetSpeed);
    }
    else {
        if (currentSpeed > 0.0f)
            currentSpeed = std::max(0.0f, currentSpeed - deceleration * deltaTime);
        else if (currentSpeed < 0.0f)
            currentSpeed = std::min(0.0f, currentSpeed + deceleration * deltaTime);
    }

    // Punch/Kick
    static bool qPress = false, ePress = false;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS && !qPress) { SwitchAnimation(punchAnim); isPunching = true; qPress = true; }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_RELEASE) { qPress = false; isPunching = false; }

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !ePress) { SwitchAnimation(kickAnim); isKicking = true; ePress = true; }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE) { ePress = false; isKicking = false; }

    // Jump (one-shot)
    static bool spaceHeld = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!spaceHeld) { TriggerJumpOnce(); spaceHeld = true; }
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        spaceHeld = false;
    }
}

// ---------- callbacks ----------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    // ไม่เปลี่ยน behavior เดิม: เมาส์ยังคุม FreeCam เท่านั้น
    float x = (float)xpos, y = (float)ypos;
    if (firstMouse) { lastX = x; lastY = y; firstMouse = false; }
    float xoff = x - lastX, yoff = lastY - y;
    lastX = x; lastY = y;

    if (!followCam) {
        camera.ProcessMouseMovement(xoff, yoff);
    }
}

void scroll_callback(GLFWwindow* window, double /*xoffset*/, double yoffset)
{
    if (followCam) {
        camDistance = std::clamp(camDistance - static_cast<float>(yoffset) * 0.3f, 1.5f, 8.0f);
    }
    else {
        camera.ProcessMouseScroll(static_cast<float>(yoffset));
    }
}
