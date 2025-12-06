// Include C++ headers
#include <iostream>
#include <string>

// Include GLEW
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

#define W_WIDTH 1920
#define W_HEIGHT 1080
#define TITLE "Super Snail"
#define SHADOW_WIDTH 1920
#define SHADOW_HEIGHT 1920
#define MATERIALS

// Global variables
GLFWwindow* window;
Camera* camera;
GLuint shaderProgram, shaderProgramnoshadows, shadowLoader;
GLuint projectionMatrixLocation, viewMatrixLocation, modelMatrixLocation;

GLuint lightVPLocation;
GLuint LaLocation, LdLocation, LsLocation, lightPositionLocation, lightPowerLocation;
GLuint KdLocation, KsLocation, KaLocation, NsLocation;

GLuint diffuseColorSampler, snailColorSampler;
GLuint specularColorSampler;
GLuint modelDiffuseTexture, snailDiffuseTexture;
GLuint useTextureLocation;

GLuint depthMapSampler;
GLuint shadowViewProjectionLocation;
GLuint shadowModelLocation;
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
    vec4{ 1, 1, 1, 1},
    vec4{ 1, 1, 1, 1},
    vec4{ 1, 1, 1, 1},
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
    //shaderProgramnoshadows = loadShaders("shaders/StandardShading.vertexshader","shaders/StandardShading.fragmentshader");
    shaderProgram = loadShaders("shaders/ShadowMapping.vertexshader", "shaders/ShadowMapping.fragmentshader");
    shadowLoader = loadShaders( "shaders/Depth.vertexshader", "shaders/Depth.fragmentshader");



    projectionMatrixLocation = glGetUniformLocation(shaderProgram, "P");
    viewMatrixLocation = glGetUniformLocation(shaderProgram, "V");
    modelMatrixLocation = glGetUniformLocation(shaderProgram, "M");
    
    KaLocation = glGetUniformLocation(shaderProgram, "mtl.Ka");
    KdLocation = glGetUniformLocation(shaderProgram, "mtl.Kd");
    KsLocation = glGetUniformLocation(shaderProgram, "mtl.Ks");
    NsLocation = glGetUniformLocation(shaderProgram, "mtl.Ns");
    LaLocation = glGetUniformLocation(shaderProgram, "light.La");
    LdLocation = glGetUniformLocation(shaderProgram, "light.Ld");
    LsLocation = glGetUniformLocation(shaderProgram, "light.Ls");
    lightPositionLocation = glGetUniformLocation(shaderProgram, "light.lightPosition_worldspace");
    lightPowerLocation = glGetUniformLocation(shaderProgram, "light.power");
    diffuseColorSampler = glGetUniformLocation(shaderProgram, "diffuseColorSampler");
    specularColorSampler = glGetUniformLocation(shaderProgram, "specularColorSampler");
    useTextureLocation = glGetUniformLocation(shaderProgram, "useTexture");

    snailColorSampler = glGetUniformLocation(shaderProgram, "snailColorSampler");
    depthMapSampler = glGetUniformLocation(shaderProgram, "shadowMapSampler");
    lightVPLocation = glGetUniformLocation(shaderProgram, "lightVP");

    shadowViewProjectionLocation = glGetUniformLocation(shadowLoader, "VP");
    shadowModelLocation = glGetUniformLocation(shadowLoader, "M");


    Heightmap::HillAlgorithmParameters params(100, 100, 200, 0, 20, 0.01f, 0.6f,300.0f,50.0f); //rows,columns,noHills,rmin,rmax,minheight,maxheight
    terrain = new Heightmap(params);

    modelDiffuseTexture = loadSOIL("models/grass.bmp");

    snailDiffuseTexture = loadSOIL("models/Tex_Snail.bmp");



    glGenFramebuffers(1, &depthFBO);
    // Binding the framebuffer, all changes bellow will affect the binded framebuffer
    // **Don't forget to bind the default framebuffer at the end of initialization
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);


    // We need a texture to store the depth image
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    // Telling opengl the required information about the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);							// Task 4.5 Texture wrapping methods
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);							// GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    // Task 4.5 Don't shadow area out of light's 
    // Step 1 : (Don't forget to comment out the respective lines above
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // Set color to set out of border 
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // Next go to fragment shader and add an iff statement, so if the distance in the z-buffer is equal to 1, 
    // meaning that the fragment is out of the texture border (or further than the far clip plane) 
    // then the shadow value is 0.
    // Attaching the texture to the framebuffer, so that it will monitor the depth component
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);


    // Since the depth buffer is only for the generation of the depth texture, 
    // there is no need to have a color output
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);


    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glfwTerminate();
        throw runtime_error("Frame buffer not initialized correctly");
    }
    // Binding the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    vec3 initPos = vec3(0.0f, 60.0f, 0.0f);
    snail = new Snail(initPos,vec3(0.0f,0.0f,0.0f),2.0f,0.2f, initPos, 1.0f,0.1f);
    
}



void drawTerrain(GLuint modelLocation) {
    mat4 modelMatrix = terrain->returnplaneMatrix();
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
    glActiveTexture(GL_TEXTURE0);								// Activate texture position
    glBindTexture(GL_TEXTURE_2D, modelDiffuseTexture);			// Assign texture to position 
    glUniform1i(diffuseColorSampler, 0);
    terrain->bind();
    terrain->draw();
}

void drawSnail(GLuint modelLocation) {
    mat4 modelMatrix = snail->snailModelMatrix;
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, snailDiffuseTexture);
    glUniform1i(snailColorSampler, 0);
    snail->draw();
}

void depth_pass(mat4 viewMatrix, mat4 projectionMatrix) {

    // Setting viewport to shadow map size
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

    // Binding the depth framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);

    // Cleaning the framebuffer depth information (stored from the last render)
    glClear(GL_DEPTH_BUFFER_BIT);

    // Selecting the new shader program that will output the depth component
    glUseProgram(shadowLoader);

    // sending the view and projection matrix to the shader
    mat4 view_projection = projectionMatrix * viewMatrix;
    glUniformMatrix4fv(shadowViewProjectionLocation, 1, GL_FALSE, &view_projection[0][0]);


    // ---- rendering the scene ---- //
    // creating suzanne model matrix and sending to GPU
    drawTerrain(shadowModelLocation);
    drawSnail(shadowModelLocation);
    

    mat4 modelMatrix = mat4();
    // binding the default framebuffer again
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //*/
}

void lighting_pass(mat4 viewMatrix, mat4 projectionMatrix) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, W_WIDTH, W_HEIGHT);

    // Step 2: Clearing color and depth info
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Step 3: Selecting shader program
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

    uploadLight(*light);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(depthMapSampler, 2);

    
    
    mat4 lightVP = light->lightVP();
    glUniformMatrix4fv(lightVPLocation, 1, GL_FALSE, &lightVP[0][0]);
    glUniform1i(useTextureLocation, 1);
    drawTerrain(modelMatrixLocation);
    drawSnail(modelMatrixLocation);

}

void free() {

    delete terrain;
    glDeleteProgram(shaderProgram);
    glfwTerminate();
}

void mainLoop() {
    float t = glfwGetTime();
    float camDist = 35.0f;
    float camHeight = 20.0f;
    // State flag to track if we are on the ground
    bool isGrounded = false;
    camera->position = vec3(0.0f,20.0f,0.0f);
    do {
        float currentTime = glfwGetTime();
        float dt = currentTime - t;

        // Safety: Prevent huge dt spikes (lag) from breaking physics
        if (dt > 0.1f) dt = 0.1f;

        glViewport(0, 0, W_WIDTH, W_HEIGHT);

        // 1. Camera Update
        camera->update();
        glm::vec3 snailForward = snail->q * glm::vec3(0, 0, 1);
        glm::vec3 snailUp = snail->q * glm::vec3(0, 1, 0);
        glm::vec3 snailRight = snail->q * glm::vec3(-1, 0, 0);
        // Calculate target position
        glm::vec3 cameraOffset = glm::vec3(0, camHeight, -camDist);
        glm::vec3 rotatedOffset = snail->q * cameraOffset;
        // snail->x + rotatedOffset; // Zoomed out a bit to see better
        //camera->viewMatrix = lookAt(camera->position, snail->x,snailUp);

        // 2. Physics Force Definition
        snail->forcing = [&](float t, const vector<float>& y)->vector<float> {
            vector<float> f(6, 0.0f);

            // LOGIC: Only apply gravity if we are NOT grounded.
            // If we are grounded, the collision snap handles the position, 
            // so we don't want gravity pulling us off the wall/cliff.
            if (!isGrounded) {
                f[1] = snail->m * g * (-1);
            }

            return f;
            };

        // 3. Update Physics (Step 1)
        snail->update(t, dt);

        // 4. Check Collision & Update 'isGrounded' (Step 2)
        // This snaps the snail to the floor if close enough
        isGrounded = handleSnailTerrainCollision(snail, terrain);

        // 5. Input Handling (Movement)
        // LOGIC: Only allow movement if grounded
        if (isGrounded) {

            // Get directions relative to the snail's rotation (latching)
            // Note: Check if your model faces -Z or +Z. I used -Z here based on your collision code.

            float turnSpeed = glm::radians(2.0f);
            float moveSpeed = 200.0f * dt; // Adjust speed as needed
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                moveSpeed *= 5.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) snail->v += snailForward * moveSpeed;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) snail->v -= snailForward * moveSpeed;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                glm::quat turn = glm::angleAxis(-turnSpeed, glm::vec3(0, 1, 0));
                snail->q = snail->q * turn * dt;
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                glm::quat turn = glm::angleAxis(turnSpeed, glm::vec3(0, 1, 0));
                snail->q = snail->q * turn * dt;

            }

            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
                if (camDist > 10) camDist -= 20 * dt;
                else camDist = 10;
            }

            // Zoom out if down arrow key is pressed
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                if (camDist < 100) camDist += 20 * dt;
                else camDist = 100;
            }

            // Sync momentum after adding input velocity
            snail->P = snail->v * snail->m;
            snail->q = glm::normalize(snail->q);

        } 

        // 6. Rendering
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        depth_pass(light->viewMatrix, light->projectionMatrix);

        glUseProgram(shaderProgram);
        uploadLight(*light);

        mat4 projectionMatrix = camera->projectionMatrix;
        mat4 viewMatrix = camera->viewMatrix;
        lighting_pass(viewMatrix, projectionMatrix);

        t += dt;

        glfwSwapBuffers(window);
        glfwPollEvents();

    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0);
}



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

    // Log
    logGLParameters();

    // Create camera
    camera = new Camera(window);
}

void uploadMaterial(const Material& mtl) {
    glUniform4f(KaLocation, mtl.Ka.r, mtl.Ka.g, mtl.Ka.b, mtl.Ka.a);
    glUniform4f(KdLocation, mtl.Kd.r, mtl.Kd.g, mtl.Kd.b, mtl.Kd.a);
    glUniform4f(KsLocation, mtl.Ks.r, mtl.Ks.g, mtl.Ks.b, mtl.Ks.a);
    glUniform1f(NsLocation, mtl.Ns);
}

void uploadLight(const Light& light) {
    glUniform4f(LaLocation, light.La.r, light.La.g, light.La.b, light.La.a);
    glUniform4f(LdLocation, light.Ld.r, light.Ld.g, light.Ld.b, light.Ld.a);
    glUniform4f(LsLocation, light.Ls.r, light.Ls.g, light.Ls.b, light.Ls.a);
    glUniform3f(lightPositionLocation, light.lightPosition_worldspace.x,
        light.lightPosition_worldspace.y, light.lightPosition_worldspace.z);
    glUniform1f(lightPowerLocation,100.0f);
}

int main(void) {
    try {
        initialize();
        createContext();
        mainLoop();
        free();
    } catch (exception& ex) {
        cout << ex.what() << endl;
        getchar();
        free();
        return -1;
    }

    return 0;
}