#include <iostream>
#include <string>

#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Shader loading utilities and other
#include <common/shader.h>
#include <common/util.h>
#include <common/camera.h>
#include <common/light.h>
#include "Snail.h"
#include "heightmap.h"
#include "Cube.h"
#include "Sphere.h"
#include "Box.h"
#include "MassSpringDamper.h"
#include "Collision.h"
#include <common/texture.h>

using namespace std;
using namespace glm;

// Function prototypes
void initialize();
void createContext();
void mainLoop();
void free();
struct Light; struct Material;
void uploadMaterial(const Material& mtl);
void uploadLight(const Light& light);
void uploadSnailMaterial(const Material& mtl);
void uploadSnailLight(const Light& light);

#define W_WIDTH 1920
#define W_HEIGHT 1080
#define TITLE "Super Snail"
#define SHADOW_WIDTH 1920
#define SHADOW_HEIGHT 1920
#define MATERIALS

// Global variables
GLFWwindow* window;
Camera* camera;
// Renamed the shared shader to terrainProgram for clarity
GLuint terrainProgram, shaderProgramnoshadows, shadowLoader, snailShaderProgram;

// Terrain/General Shader Uniforms
GLuint projectionMatrixLocation, viewMatrixLocation, modelMatrixLocation;
GLuint lightVPLocation, depthMapSampler, useTextureLocation;
GLuint LaLocation, LdLocation, LsLocation, lightPositionLocation, lightPowerLocation;
GLuint KdLocation, KsLocation, KaLocation, NsLocation;
GLuint diffuseColorSampler, specularColorSampler;

// Snail Shader Specific Uniforms
GLuint snailModelMatrixLocation, snailProjectionMatrixLocation, snailViewMatrixLocation;
GLuint snailLightVPLocation, snailDepthMapSampler, snailUseTextureLocation;
GLuint snailTimeLocation, snailSpeedLocation, snailRetractFactorLocation;
GLuint snailLaLocation, snailLdLocation, snailLsLocation, snailLightPositionLocation, snailLightPowerLocation;
GLuint snailKdLocation, snailKsLocation, snailKaLocation, snailNsLocation;
GLuint snailColorSampler, snailDiffuseColorSampler, snailSpecularColorSampler;

// Shadow Pass Uniforms
GLuint shadowViewProjectionLocation;
GLuint shadowModelLocation;

GLuint modelDiffuseTexture, snailDiffuseTexture;
GLuint depthFBO, depthTexture;

struct Material {
    vec4 Ka;
    vec4 Kd;
    vec4 Ks;
    float Ns;
};

const Material defaultMaterial{
    vec4{0.1, 0.1, 0.1, 1},
    vec4{1.0, 0.8, 0.0, 1},
    vec4{0.3, 0.3, 0.3, 1},
    20.0f
};

Light* light = new Light(window,
    vec4{ 1, 1, 1, 1 },
    vec4{ 1, 1, 1, 1 },
    vec4{ 1, 1, 1, 1 },
    vec3(0, 100, 0)
);

Cube* cube;
Sphere* sphere[3];
Box* box;
MassSpringDamper* msd;
Heightmap* terrain;
Snail* snail;

// Standard acceleration due to gravity
const float g = 9.80665f;

void createContext() {
    // Load Shaders
    terrainProgram = loadShaders("shaders/ShadowMapping.vertexshader", "shaders/ShadowMapping.fragmentshader"); // Used for Terrain
    shadowLoader = loadShaders("shaders/Depth.vertexshader", "shaders/Depth.fragmentshader");
    snailShaderProgram = loadShaders("shaders/snail.vertexshader", "shaders/snail.fragmentshader"); // New Snail Shader

    // --- Terrain/General Shader Uniforms ---
    projectionMatrixLocation = glGetUniformLocation(terrainProgram, "P");
    viewMatrixLocation = glGetUniformLocation(terrainProgram, "V");
    modelMatrixLocation = glGetUniformLocation(terrainProgram, "M");
    lightVPLocation = glGetUniformLocation(terrainProgram, "lightVP");
    depthMapSampler = glGetUniformLocation(terrainProgram, "shadowMapSampler");
    useTextureLocation = glGetUniformLocation(terrainProgram, "useTexture");

    KaLocation = glGetUniformLocation(terrainProgram, "mtl.Ka");
    KdLocation = glGetUniformLocation(terrainProgram, "mtl.Kd");
    KsLocation = glGetUniformLocation(terrainProgram, "mtl.Ks");
    NsLocation = glGetUniformLocation(terrainProgram, "mtl.Ns");
    LaLocation = glGetUniformLocation(terrainProgram, "light.La");
    LdLocation = glGetUniformLocation(terrainProgram, "light.Ld");
    LsLocation = glGetUniformLocation(terrainProgram, "light.Ls");
    lightPositionLocation = glGetUniformLocation(terrainProgram, "light.lightPosition_worldspace");
    lightPowerLocation = glGetUniformLocation(terrainProgram, "light.power");
    diffuseColorSampler = glGetUniformLocation(terrainProgram, "diffuseColorSampler");
    specularColorSampler = glGetUniformLocation(terrainProgram, "specularColorSampler");

    // --- Snail Shader Uniforms ---
    snailProjectionMatrixLocation = glGetUniformLocation(snailShaderProgram, "P");
    snailViewMatrixLocation = glGetUniformLocation(snailShaderProgram, "V");
    snailModelMatrixLocation = glGetUniformLocation(snailShaderProgram, "M");
    snailLightVPLocation = glGetUniformLocation(snailShaderProgram, "lightVP");
    snailDepthMapSampler = glGetUniformLocation(snailShaderProgram, "shadowMapSampler");
    snailColorSampler = glGetUniformLocation(snailShaderProgram, "diffuseColorSampler");

    // New Animation Uniforms
    snailTimeLocation = glGetUniformLocation(snailShaderProgram, "time");
    snailSpeedLocation = glGetUniformLocation(snailShaderProgram, "snailSpeed");
    snailRetractFactorLocation = glGetUniformLocation(snailShaderProgram, "retractFactor");

    snailKaLocation = glGetUniformLocation(snailShaderProgram, "mtl.Ka");
    snailKdLocation = glGetUniformLocation(snailShaderProgram, "mtl.Kd");
    snailKsLocation = glGetUniformLocation(snailShaderProgram, "mtl.Ks");
    snailNsLocation = glGetUniformLocation(snailShaderProgram, "mtl.Ns");
    snailLaLocation = glGetUniformLocation(snailShaderProgram, "light.La");
    snailLdLocation = glGetUniformLocation(snailShaderProgram, "light.Ld");
    snailLsLocation = glGetUniformLocation(snailShaderProgram, "light.Ls");
    snailLightPositionLocation = glGetUniformLocation(snailShaderProgram, "light.lightPosition_worldspace");
    snailLightPowerLocation = glGetUniformLocation(snailShaderProgram, "light.power");
	snailUseTextureLocation = glGetUniformLocation(snailShaderProgram, "useTexture");
    // --- Shadow Loader Uniforms ---
    shadowViewProjectionLocation = glGetUniformLocation(shadowLoader, "VP");
    shadowModelLocation = glGetUniformLocation(shadowLoader, "M");


    Heightmap::HillAlgorithmParameters params(100, 100, 200, 0, 20, 0.01f, 0.6f, 300.0f, 50.0f);
    terrain = new Heightmap(params);

    modelDiffuseTexture = loadSOIL("models/grass.bmp");
    snailDiffuseTexture = loadSOIL("models/Tex_Snail.bmp");

    // --- Depth Buffer Setup (Unchanged) ---
    glGenFramebuffers(1, &depthFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glfwTerminate();
        throw runtime_error("Frame buffer not initialized correctly");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Snail Initialization (Updated to use getHeightAt)
    float spawnX = 0.0f;
    float spawnZ = 0.0f;
    float spawnY = terrain->getHeightAt(spawnX, spawnZ);
    vec3 initPos = vec3(spawnX, spawnY + 1.0f, spawnZ);
    snail = new Snail(initPos, vec3(0.0f, 0.0f, 0.0f), 1.0f, 0.2f, initPos, 1.0f, 0.1f);
}

void drawTerrain(GLuint program, GLuint modelLocation, GLuint diffuseSampler) {
    glUseProgram(program);

    // Set Terrain Specific Uniforms (Matrices and LightVP are set in lighting_pass)
    uploadLight(*light); // Must upload light for this shader
    uploadMaterial(defaultMaterial); // Upload default material for terrain

    mat4 modelMatrix = terrain->returnplaneMatrix();
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, modelDiffuseTexture);
    glUniform1i(diffuseSampler, 0);
    glUniform1i(useTextureLocation, 1); // Tell terrain shader to use texture

    terrain->bind();
    terrain->draw();
}

void drawSnail(GLuint program, GLuint modelLocation, float time, float speed, float retractFactor) {
    glUseProgram(program);

    // Set Snail Specific Uniforms
    uploadSnailLight(*light); // Must upload light for this shader
    uploadSnailMaterial(defaultMaterial); // Upload default material for snail (optional, you might set a specific snail material)

    // Animation Uniforms
    glUniform1f(snailTimeLocation, time);
    glUniform1f(snailSpeedLocation, speed);
    glUniform1f(snailRetractFactorLocation, retractFactor);


    mat4 modelMatrix = snail->snailModelMatrix;
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
    glUniform1f(snailUseTextureLocation, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, snailDiffuseTexture);
    glUniform1i(snailColorSampler, 0); // Assuming snail texture uses snailColorSampler (unit 0)

    // Shadow Map Binding (Use Texture Unit 2, as defined in lighting_pass)
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(snailDepthMapSampler, 2);


    // Light VP (Shadow Projection)
    mat4 lightVP = light->lightVP();
    glUniformMatrix4fv(snailLightVPLocation, 1, GL_FALSE, &lightVP[0][0]);

    snail->draw();
}

void depth_pass() { // Removed matrix arguments, they are taken from light->lightVP()
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Use Shadow Loader for all geometry
    glUseProgram(shadowLoader);

    mat4 lightVP = light->lightVP();
    glUniformMatrix4fv(shadowViewProjectionLocation, 1, GL_FALSE, &lightVP[0][0]);

    // ---- rendering the scene for depth ---- //
    drawTerrain(shadowLoader, shadowModelLocation, diffuseColorSampler); // Draw terrain model

    // Draw Snail (no animation uniforms needed here, just position)
    mat4 snailModelMatrix = snail->snailModelMatrix;
    glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &snailModelMatrix[0][0]);
    snail->draw();


    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void lighting_pass(mat4 viewMatrix, mat4 projectionMatrix) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, W_WIDTH, W_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- RENDER TERRAIN (using terrainProgram) ---
    glUseProgram(terrainProgram);
    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

    // Set Shared Shadow and Light Uniforms
    mat4 lightVP = light->lightVP();
    glUniformMatrix4fv(lightVPLocation, 1, GL_FALSE, &lightVP[0][0]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(depthMapSampler, 2);

    drawTerrain(terrainProgram, modelMatrixLocation, diffuseColorSampler);


    // --- RENDER SNAIL (using snailShaderProgram) ---
    glUseProgram(snailShaderProgram);
    glUniformMatrix4fv(snailViewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(snailProjectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

    // Draw Snail with animation parameters
    float snailSpeed = length(snail->v); // magnitude of velocity for creep animation
    float retractFactor = 0.0f; // placeholder, update in mainLoop later

    drawSnail(snailShaderProgram, snailModelMatrixLocation, (float)glfwGetTime(), snailSpeed, retractFactor);
}

void free() {
    delete terrain;
    glDeleteProgram(terrainProgram);
    glDeleteProgram(snailShaderProgram); // Cleanup new shader
    glDeleteProgram(shadowLoader);
    glfwTerminate();
}

void mainLoop() {
    float t = glfwGetTime();
    float camDist = 35.0f;
    float camHeight = 20.0f;
    bool isGrounded = false;

    // Flags to check for user input (for sliding fix)
    bool isMoving = false;

    // Snail Retract/Animation Variables
    float retractTarget = 0.0f; // Target for retracting (1.0 = fully retracted)
    float retractCurrent = 0.0f; // Current animation progress (0.0 to 1.0)
    const float retractSpeed = 2.0f; // Speed of retract/extend animation per second

    camera->position = vec3(0.0f, 20.0f, 0.0f);
    do {
        float currentTime = glfwGetTime();
        float dt = currentTime - t;
        if (dt > 0.1f) dt = 0.1f;

        glViewport(0, 0, W_WIDTH, W_HEIGHT);
        camera->update(snail);

        // --- 1. Input Handling and Velocity ---
        isMoving = false;

        // Check for movement keys
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            isMoving = true;
        }

        // Check for Retract Key (Step 3 Setup)
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            retractTarget = 1.0f;
            isMoving = false; // Cannot move while retracting
        }
        else {
            retractTarget = 0.0f;
        }

        // Animation Update (smooth transition)
        if (retractCurrent < retractTarget) {
            retractCurrent += retractSpeed * dt;
        }
        else if (retractCurrent > retractTarget) {
            retractCurrent -= retractSpeed * dt;
        }
        retractCurrent = glm::clamp(retractCurrent, 0.0f, 1.0f);


        vec3 snailForward = snail->q * vec3(0, 0, -1); // Assuming -Z is forward

        // --- 2. Physics Update (Forcing) ---
        snail->forcing = [&](float t, const vector<float>& y)->vector<float> {
            vector<float> f(6, 0.0f);

            // Apply gravity only if not grounded (or if retracted)
            if (!isGrounded && retractCurrent < 0.9f) { // Added tolerance for retract check
                f[1] = snail->m * g * (-1);
            }

            // Add air resistance/damping if retracted
            if (retractCurrent > 0.9f) {
                // simple damping: f = -k * v
                f[0] = -snail->v.x * snail->m * 5.0f;
                f[1] = -snail->v.y * snail->m * 5.0f;
                f[2] = -snail->v.z * snail->m * 5.0f;
            }

            return f;
            };

        isGrounded = handleSnailTerrainCollision(snail, terrain, !isMoving);

        // --- 4. Movement Input ---
        if (isGrounded && isMoving && retractCurrent < 0.9f) { // Cannot move when retracted
            float turnSpeed = radians(100.0f) * dt; // Increased turn speed for better feel
            float moveSpeed = 5.0f;

            // Allow movement only in snail's current orientation
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) snail->v -= snailForward * moveSpeed;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) snail->v += snailForward * moveSpeed * 0.5f; // Slower reverse

            // Rotation (Yaw)
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                quat turn = angleAxis(-turnSpeed, vec3(0, 1, 0));
                snail->q = normalize(snail->q * turn);
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                quat turn = angleAxis(turnSpeed, vec3(0, 1, 0));
                snail->q = normalize(snail->q * turn);
            }

            // Sync momentum after adding input velocity
            snail->P = snail->v * snail->m;
        }

        // Apply physics step
        snail->update(t, dt);

        // --- 5. Rendering ---
        depth_pass(); // Use light matrices implicitly

        mat4 projectionMatrix = camera->projectionMatrix;
        mat4 viewMatrix = camera->viewMatrix;
        lighting_pass(viewMatrix, projectionMatrix);

        t += dt;
        glfwSwapBuffers(window);
        glfwPollEvents();

    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0);
}

// ... [initialize, uploadMaterial, uploadLight, main function are unchanged, except for the name change from shaderProgram to terrainProgram in uploadLight/uploadMaterial] ...

void initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        throw runtime_error("Failed to initialize GLFW\n");
    }


    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(W_WIDTH, W_HEIGHT, TITLE, NULL, NULL);

    if (window == NULL) {
        glfwTerminate();
        throw runtime_error(string(string("Failed to open GLFW window.") +
            " If you have an Intel GPU, they are not 3.3 compatible." +
            "Try the 2.1 version.\n"));
    }
    glfwMakeContextCurrent(window);
    // Start GLEW extension handler
    glewExperimental = GL_TRUE;
    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        throw runtime_error("Failed to initialize GLEW\n");
    }
    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited movement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, W_WIDTH / 2, W_HEIGHT / 2);
    // Gray background color
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);
    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);
    // glFrontFace(GL_CW);
    // glFrontFace(GL_CCW);
    // enable point size when drawing points
    glEnable(GL_PROGRAM_POINT_SIZE);
    logGLParameters();
    camera = new Camera(window);

}

void uploadMaterial(const Material& mtl) {
    glUniform4f(KaLocation, mtl.Ka.r, mtl.Ka.g, mtl.Ka.b, mtl.Ka.a); // Assuming KaLocation is the latest bound uniform
    glUniform4f(KdLocation, mtl.Kd.r, mtl.Kd.g, mtl.Kd.b, mtl.Kd.a);
    glUniform4f(KsLocation, mtl.Ks.r, mtl.Ks.g, mtl.Ks.b, mtl.Ks.a);
    glUniform1f(NsLocation, mtl.Ns);
}

void uploadSnailMaterial(const Material& mtl) {
    glUniform4f(snailKaLocation, mtl.Ka.r, mtl.Ka.g, mtl.Ka.b, mtl.Ka.a); // Assuming KaLocation is the latest bound uniform
    glUniform4f(snailKdLocation, mtl.Kd.r, mtl.Kd.g, mtl.Kd.b, mtl.Kd.a);
    glUniform4f(snailKsLocation, mtl.Ks.r, mtl.Ks.g, mtl.Ks.b, mtl.Ks.a);
    glUniform1f(snailNsLocation, mtl.Ns);
}


void uploadLight(const Light& light) {
    glUniform4f(LaLocation, light.La.r, light.La.g, light.La.b, light.La.a); // Assuming LaLocation is the latest bound uniform
    glUniform4f(LdLocation, light.Ld.r, light.Ld.g, light.Ld.b, light.Ld.a);
    glUniform4f(LsLocation, light.Ls.r, light.Ls.g, light.Ls.b, light.Ls.a);
    glUniform3f(lightPositionLocation, light.lightPosition_worldspace.x,
        light.lightPosition_worldspace.y, light.lightPosition_worldspace.z);
    glUniform1f(lightPowerLocation, 100.0f);
}

void uploadSnailLight(const Light& light) {
    glUniform4f(snailLaLocation, light.La.r, light.La.g, light.La.b, light.La.a); // Assuming LaLocation is the latest bound uniform
    glUniform4f(snailLdLocation, light.Ld.r, light.Ld.g, light.Ld.b, light.Ld.a);
    glUniform4f(snailLsLocation, light.Ls.r, light.Ls.g, light.Ls.b, light.Ls.a);
    glUniform3f(snailLightPositionLocation, light.lightPosition_worldspace.x,
        light.lightPosition_worldspace.y, light.lightPosition_worldspace.z);
    glUniform1f(snailLightPowerLocation, 100.0f);
}

int main(void) {
    try {
        initialize();
        createContext();
        mainLoop();
        free();
    }
    catch (exception& ex) {
        cout << ex.what() << endl;
        getchar();
        free();
        return -1;
    }

    return 0;
}