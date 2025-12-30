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
#include "Flower.h"
#include "MassSpringDamper.h"
#include "Collision.h"
#include <common/model.h>
#include <common/texture.h>
#include <stb_image_aug.h>
#include "Menu.h"

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

#define W_WIDTH 1024
#define W_HEIGHT 720
#define TITLE "Super Snail"
#define SHADOW_WIDTH 2048
#define SHADOW_HEIGHT 2048
#define MATERIALS

// Global variables
GLFWwindow* window;
Camera* camera;
// Renamed the shared shader to terrainProgram for clarity
GLuint terrainProgram, flowerShading, shadowLoader, snailShaderProgram,skyboxShader, vegetShader;

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
//skybox
GLuint skyProjectionMatrixLocation, skyViewMatrixLocation;
GLuint skyboxVAO, skyboxVBO;
GLuint cubemapTexture;

//stamina bar
GLuint staminaShader;
GLint modelLoc, colorLoc,useTextureMenuLoc;  

//instanced rendering
GLuint treeVAO,instanceVBO;
GLuint treeTexture;
vector<vec3> treeVertices;
vector<vec2> treeUVs;
vector<vec3> treeNormals;
int treeVertexCount = 0; 

//LOULOUDIAAAAAA (me powerups)
Flower* redFlower,* purpulFlower, * pizza, *mushroom, *mushroom2;

//menu
enum GameState { MENU_STATE, GAME_STATE };
GameState currentState;
Menu* mainMenu;

struct Material {
    vec4 Ka; 
    vec4 Kd;
    vec4 Ks; 
    float Ns;     
    string map_Kd; 
    GLuint textureID;   
};


Light* light = new Light(window,
    vec4{ 1, 1, 1, 1 },
    vec4{ 1, 1, 1, 1 },
    vec4{ 1, 1, 1, 1 },
    vec3(0, 200, 0)
);

Cube* cube;
Sphere* sphere[3];
Box* box;
MassSpringDamper* msd;
Heightmap* terrain;
Snail* snail;
Drawable* quad;


vector<mat4> instanceMatrices;
int treeCount = 50;

// Standard acceleration due to gravity
const float g = 9.80665f;

//load skybox faces

unsigned int loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void initSkybox() {
    std::vector<std::string> faces = {
        "skybox/posx.jpg", //Right
        "skybox/negx.jpg", //Left
        "skybox/posy.jpg", //Top
        "skybox/negy.jpg", //Bottom
        "skybox/posz.jpg", //Front
        "skybox/negz.jpg"  //Back
    };
    cubemapTexture = loadCubemap(faces);

    float skyboxVertices[] = {
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
    // skybox VAO and VBO
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void initUI() {
    
    vector<vec3> quadVertices = {
        vec3(0.0, 0.0, 0.0), // bottom-left
        vec3(1.0, 0.0, 0.0), // bottom-right
        vec3(1.0, 1.0, 0.0), // top-right
        vec3(1.0, 1.0, 0.0), // top-right
        vec3(0.0, 1.0, 0.0), // top-left
        vec3(0.0, 0.0, 0.0)  // bottom-left
    };

    vector<vec2> quadUVs = {
      vec2(0.0, 0.0),
      vec2(1.0, 0.0),
      vec2(1.0, 1.0),
      vec2(1.0, 1.0),
      vec2(0.0, 1.0),
      vec2(0.0, 0.0)
    };

    quad = new Drawable(quadVertices, quadUVs);
}

void generateTreePositions(int amount) {

    for (int i = 0; i < amount; i++) {
        float x = (rand() % 1000 - 500); // Random X
        float z = (rand() % 1000 - 500); // Random Z
        float y = terrain->getHeightAt(x, z);    // Get Y from heightmap

        mat4 model = translate(mat4(1.0f), vec3(x, y, z));
        model = rotate(model, radians((float)(rand() % 360)), vec3(0, 1, 0)); // Random rotation
        float scaleFactor = 0.1f;
        model = scale(model, vec3(scaleFactor, scaleFactor, scaleFactor));
        instanceMatrices.push_back(model);
    }

}

void initVegetationBuffers(GLuint treeVAO) {
    generateTreePositions(treeCount);
    
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, treeCount * sizeof(mat4), &instanceMatrices[0], GL_STATIC_DRAW);

    // Bind to the VAO of your tree model
    glBindVertexArray(treeVAO);

    // A mat4 is 4 vec4s, so it takes 4 attribute slots (locations 3, 4, 5, 6)
    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(i * sizeof(vec4)));
        // Tell OpenGL this attribute changes per instance, not per vertex
        glVertexAttribDivisor(3 + i, 1);
    }
    glBindVertexArray(0);
}

void initTree() {
	GLuint vertexVBO, uvVBO, normalVBO;
    // 1. Load the Model
    // Ensure you use the try-catch block if your loader throws exceptions
    try {
        loadOBJWithTiny("models/tree.obj", treeVertices, treeUVs, treeNormals);
    }
    catch (...) {
        std::cout << "Failed to load tree.obj" << std::endl;
        return;
    }

    if (treeVertices.empty()) {
        std::cout << "Tree model has no vertices!" << std::endl;
        return;
    }

    treeVertexCount = treeVertices.size();

    // 2. Load the Texture
    treeTexture = loadSOIL("models/tree.bmp");

    // 3. Setup the VAO
    glGenVertexArrays(1, &treeVAO);
    glBindVertexArray(treeVAO);

    // --- Vertices (Attribute 0) ---
    // Safe check: We know vertices exist because of the check above, but good practice.
    glGenBuffers(1, &vertexVBO);
    glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
    glBufferData(GL_ARRAY_BUFFER, treeVertices.size() * sizeof(vec3), &treeVertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    // --- UVs (Attribute 1) ---
    // FIX IS HERE: Only create UV buffer if UVs actually exist
    if (!treeUVs.empty()) {
        glGenBuffers(1, &uvVBO);
        glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
        glBufferData(GL_ARRAY_BUFFER, treeUVs.size() * sizeof(vec2), &treeUVs[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    else {
        std::cout << "Warning: tree.obj has no UVs. Texture will not work." << std::endl;
    }

    // --- Normals (Attribute 2) ---
    // FIX IS HERE: Only create Normal buffer if Normals actually exist
    if (treeNormals.empty()) {
        std::cout << "Generating dummy normals for trees..." << std::endl;
        for (size_t i = 0; i < treeVertices.size(); i++) {
            // Point straight up (0,1,0) so they catch the sunlight
            treeNormals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }
    glGenBuffers(1, &normalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, treeNormals.size() * sizeof(vec3), &treeNormals[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    // 4. Setup Instances
    // Pass the VAO to link the instance buffer
    initVegetationBuffers(treeVAO);

    // Unbind
    glBindVertexArray(0);
}


void createContext() {
    currentState = MENU_STATE;
    // Load Shaders
    terrainProgram = loadShaders("shaders/ShadowMapping.vertexshader", "shaders/ShadowMapping.fragmentshader"); // Used for Terrain
    shadowLoader = loadShaders("shaders/Depth.vertexshader", "shaders/Depth.fragmentshader");
    snailShaderProgram = loadShaders("shaders/snail.vertexshader", "shaders/snail.fragmentshader"); // New Snail Shader
	skyboxShader = loadShaders("shaders/skybox.vertexshader", "shaders/skybox.fragmentshader");
    staminaShader = loadShaders("shaders/ui.vertexshader", "shaders/ui.fragmentshader");
	vegetShader = loadShaders("shaders/veget.vertexshader", "shaders/veget.fragmentshader");
	flowerShading = loadShaders("shaders/flower.vertexshader", "shaders/flower.fragmentshader");
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
	//skybox uniforms
	skyViewMatrixLocation = glGetUniformLocation(skyboxShader, "view");
	skyProjectionMatrixLocation = glGetUniformLocation(skyboxShader, "projection");
    modelLoc = glGetUniformLocation(staminaShader, "model");
    colorLoc = glGetUniformLocation(staminaShader, "barColor");
    useTextureMenuLoc = glGetUniformLocation(staminaShader, "useTexture");

	// Terrain Initialization
	Heightmap::HillAlgorithmParameters params(400, 400, 100, 10, 40, 0.0f, 1.0f, 1500.0f, 50.0f);
    // rows, columns, numHills, hillRadiusMin, hillRadiusMax, hillMinHeight, hillMaxHeight, scalar, scalarY
    terrain = new Heightmap(params);

    modelDiffuseTexture = loadSOIL("models/grass_minecraft.bmp");
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
    snail = new Snail(initPos, 1.0f, 0.2f);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //skyboxx
	initSkybox();

    //stamina bar
    initUI();
    initTree();

	//flowers
	redFlower = new Flower("models/flowers/redFlower.obj", "models/flowers/redFlower.mtl", terrain,20,5.0f,true);
	purpulFlower = new Flower("models/flowers/bellFlower.obj", "models/flowers/bellFlower.mtl", terrain, 10, 4.0f, true);
    mushroom = new Flower("models/flowers/mushroom.obj", "models/flowers/mushroom.mtl", terrain, 10, 0.3f, true);
    mushroom2 = new Flower("models/flowers/mushroom.obj", "models/flowers/mushroom2.mtl", terrain, 10, 0.3f, true);
    pizza = new Flower("models/flowers/pizza.obj", "models/flowers/pizza.bmp", terrain, 1, 1.0f, false);

	//menu init
    mainMenu = new Menu();
    mainMenu->init();

}

void drawTerrain(GLuint program, GLuint modelLocation, GLuint diffuseSampler) {
    glUseProgram(program);

    // Set Terrain Specific Uniforms (Matrices and LightVP are set in lighting_pass)
    uploadLight(*light); 

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
    uploadSnailLight(*light); 

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



void drawSkybox(mat4 view, mat4 projectionMatrix) {
    // 1. Change depth function so skybox passes at maximum depth (1.0)
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxShader);

    // 2. Remove translation from the view matrix
    // We convert to mat3 and back to mat4 to keep only rotation
	mat4 viewMatrix = mat4(mat3(view));
    glUniformMatrix4fv(skyViewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(skyProjectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

void drawStaminaBar(float stamina, float maxStamina) {
    glDisable(GL_DEPTH_TEST);
    glUseProgram(staminaShader);
    glUniform1i(useTextureMenuLoc, 0);
    float width = (float)W_WIDTH;
    float height = (float)W_HEIGHT;

	mat4 translateMat = translate(mat4(1.0f), vec3(-0.95f, -0.8f, 0.0f));
    float barMaxW = 200.0f;
    float barH = 20.0f;
    float percentage = glm::clamp(stamina / maxStamina, 0.0f, 1.0f);

    quad->bind();
    // --- DRAW BACKGROUND ---
    
    mat4 bgModel = translateMat * scale(mat4(1.0f), vec3(1.0f/ 2.0f,1.0/ 20.0f, 1.0f));
    // Shift by -0.5 so the 0.5 corner of your quad acts as the 0.0 origin

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &bgModel[0][0]);
    glUniform3f(colorLoc, 0.2f, 0.2f, 0.2f);
    quad->draw();

    // --- DRAW FILL ---
    mat4 fgModel = translateMat * scale(mat4(1.0f), vec3(1.0f / 2.0f * percentage, 1.0 / 20.0f, 1.0f));

    vec3 color = (percentage > 0.5f) ? vec3(0.0f, 0.8f, 0.0f) : (percentage > 0.2f ? vec3(0.8f, 0.8f, 0.0f) : vec3(0.8f, 0.0f, 0.0f));
    glUniform1i(useTextureMenuLoc, 0);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &fgModel[0][0]);
    glUniform3f(colorLoc, color.x, color.y, color.z);
    quad->draw();

    glEnable(GL_DEPTH_TEST);
}

void drawFlowers(GLuint program,bool drawShading) {
    
    redFlower->draw(program, drawShading);
    purpulFlower->draw(program, drawShading);
	mushroom->draw(program, drawShading);
    mushroom2->draw(program, drawShading);
	pizza->draw(program, drawShading);
}

void depth_pass() { 
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    // Use Shadow Loader for all geometry
    glUseProgram(shadowLoader);


    glUniform1i(glGetUniformLocation(shadowLoader, "isInstanced"), 0);
    mat4 lightVP = light->lightVP();
    glUniformMatrix4fv(shadowViewProjectionLocation, 1, GL_FALSE, &lightVP[0][0]);
    drawTerrain(shadowLoader, shadowModelLocation, diffuseColorSampler); 
    mat4 snailModelMatrix = snail->snailModelMatrix;
    glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &snailModelMatrix[0][0]);
    snail->draw();

    glUniform1i(glGetUniformLocation(shadowLoader, "isInstanced"), 1);
    glBindVertexArray(treeVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, treeVertexCount, treeCount);

    drawFlowers(shadowLoader, true);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void lighting_pass(mat4 viewMatrix, mat4 projectionMatrix, float retractFactor) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, W_WIDTH, W_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
    //draw Terrain
    glUseProgram(terrainProgram);
    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
    mat4 lightVP = light->lightVP();
    glUniformMatrix4fv(lightVPLocation, 1, GL_FALSE, &lightVP[0][0]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(depthMapSampler, 2);
    drawTerrain(terrainProgram, modelMatrixLocation, diffuseColorSampler);


    //draw Snail
    glUseProgram(snailShaderProgram);
    glUniformMatrix4fv(snailViewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(snailProjectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);    
    float snailSpeed = length(snail->v);
    drawSnail(snailShaderProgram, snailModelMatrixLocation, (float)glfwGetTime(), snailSpeed, retractFactor);
    
    glUseProgram(vegetShader);
    glUniformMatrix4fv(glGetUniformLocation(vegetShader, "P"), 1, GL_FALSE, &projectionMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(vegetShader, "V"), 1, GL_FALSE, &viewMatrix[0][0]);
    glDisable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, treeTexture);

    glBindVertexArray(treeVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, treeVertexCount, treeCount);

    glEnable(GL_CULL_FACE);

    glUseProgram(flowerShading);
    glUniformMatrix4fv(glGetUniformLocation(flowerShading, "P"), 1, GL_FALSE, &projectionMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(flowerShading, "V"), 1, GL_FALSE, &viewMatrix[0][0]);
    drawFlowers(flowerShading, false);

    glBindVertexArray(0);

}



void free() {
    delete terrain;
    glDeleteProgram(terrainProgram);
    glDeleteProgram(snailShaderProgram); // Cleanup new shader
    glDeleteProgram(shadowLoader);
    glfwTerminate();
}

void checkforFlowerCollision() {
    vector<Flower*> flowers = { redFlower, purpulFlower, mushroom, mushroom2, pizza };
    for (Flower* flower : flowers) {
		flower->checkSnailCollisionNotRetracted(snail);
       
    }
}

// - REPLACE YOUR menuLoop WITH THIS
void menuLoop() {
    double x, y;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Setup for 2D rendering
    glUseProgram(staminaShader);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // FIX 1: Add glfwWindowShouldClose check to stop infinite hangs

    do {

        // FIX 2: Clear Screen to prevent "smearing" artifacts
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw Menu
        mainMenu->draw(staminaShader, W_WIDTH, W_HEIGHT);

        // Handle Input
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            glfwGetCursorPos(window, &x, &y);
            int action = mainMenu->checkClick(x, y, W_HEIGHT);

            if (action == 1) {
                currentState = GAME_STATE; // Start Game
            }
            else if (action == 2) {
                glfwSetWindowShouldClose(window, 1); // Exit App
            }
        }

        // FIX 3: CRITICAL! Keep the window alive
        glfwSwapBuffers(window);
        glfwPollEvents();
    } while (currentState == MENU_STATE && glfwWindowShouldClose(window) == 0);

    // Cleanup before starting game
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void mainLoop() {

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(window, W_WIDTH / 2, W_HEIGHT / 2);
    float t = glfwGetTime();
    float camDist = 35.0f;
    float camHeight = 20.0f;
    bool isGrounded = false;
    bool onTree = false;
    bool isMoving = false;
    bool isControlKeyHeld = false;

    camera->position = vec3(0.0f, 20.0f, 0.0f);
    do {
        float currentTime = glfwGetTime();
        float dt = currentTime - t;
        if (dt > 0.1f) dt = 0.1f;

        glViewport(0, 0, W_WIDTH, W_HEIGHT);
        camera->update(snail);
		light->update(snail->x);
        // --- 1. Input Handling and Velocity ---
        

        // Check for Retract Key (Step 3 Setup)
        bool controlPressed = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);

        if (controlPressed && !isControlKeyHeld){
            
            if (snail->retractTarget == 0.0f) {
                snail->retractTarget = 1.0f;
            }
            else {
                snail->retractTarget = 0.0f;
            }


        }
        isControlKeyHeld = controlPressed;
        // Animation Update (smooth transition)
        if (snail->retractCurrent < snail->retractTarget) {
            snail->retractCurrent += snail->retractSpeed * dt;
        }
        else if (snail->retractCurrent > snail->retractTarget) {
            snail->retractCurrent -= snail->retractSpeed * dt;
        }
        snail->retractCurrent = clamp(snail->retractCurrent, 0.0f, 1.0f);

        if (snail->retractTarget == 0.0f) {
            snail->isRetracted = false;
        }
        else if (snail->retractCurrent == 1.0f) {
            snail->isRetracted = true;
		}


        


        // --- 2. Physics Update (Forcing) ---
        if (snail->retractCurrent == 100.0f)checkforFlowerCollision();
        onTree = handleSnailTreeCollision(snail, instanceMatrices);
        isGrounded = handleSnailTerrainCollision(snail, terrain, onTree);
        vec3 snailForward = snail->q * vec3(0, 0, -1); //-Z is forward
        vec3 snailRight = snail->q * vec3(1, 0, 0);
        snail->forcing = [&](float t, const vector<float>& y) -> vector<float>
            {
                vector<float> f(6, 0.0f);
                if (!snail->isRetracted) {
                    float stopDamping = 10.0f; // Very strong braking

                    // Linear Braking
                    f[0] = -snail->v.x * stopDamping * snail->m;
                    f[1] = -snail->v.y * stopDamping * snail->m;
                    f[2] = -snail->v.z * stopDamping * snail->m;

                    // Angular Braking (Stop spinning)
                    f[3] = -snail->w.x * stopDamping;
                    f[4] = -snail->w.y * stopDamping;
                    f[5] = -snail->w.z * stopDamping;

                    return f;
                }

                const float muK = 0.35f;
                const float muS = 0.55f;
                const float inputForce = 1.2f;
                const float BIAS = 1e-4f;

                vec3 gravity(0.0f, -snail->m * g, 0.0f);
                vec3 totalForce = gravity;
                vec3 totalTorque(0.0f);

                if (isGrounded)
                {
                    vec3 n = normalize(terrain->getNormalAt(snail->x.x, snail->x.z));
                    vec3 up(0, 1, 0);

                    // ---- Cancel normal gravity
                    vec3 gN = dot(gravity, n) * n;
                    vec3 gT = gravity - gN;
                    totalForce -= gN;

                    vec3 camForwardFlat = normalize(vec3(sin(camera->horizontalAngle), 0, cos(camera->horizontalAngle)));

                    // 2. Project camera forward onto the slope
                    vec3 slopeForward = camForwardFlat - dot(camForwardFlat, n) * n;
                    if (length(slopeForward) < BIAS) {
                        // Fallback if looking straight down/up a cliff
                        slopeForward = vec3(0, 0, -1) - dot(vec3(0, 0, -1), n) * n;
                    }
                    slopeForward = normalize(slopeForward);

                    vec3 slopeRight = normalize(cross(slopeForward, n));

                    vec3 vT = snail->v - dot(snail->v, n) * n;

                    float N = snail->m * g * clamp(dot(n, up), 0.0f, 1.0f);

                    vec3 drive(0.0f);
                    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) drive -= slopeForward; 
                    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) drive += slopeForward; 
                    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) drive -= slopeRight;   
                    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) drive += slopeRight;  
                    
                    if (length(drive) > BIAS)
                        drive = normalize(drive) * inputForce;
                    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && snail->abilityUnlocked) totalForce -= 200.0f * gN;
                    vec3 friction(0.0f);
                    if (length(vT) < 0.05f && length(drive + gT) < muS * N)
                    {
                        // STATIC friction  cancel the desire to move
                        friction = -(drive + gT);
                    }
                    else if (length(vT) > BIAS)
                    {
                        // KINETIC friction  slide
                        friction = -normalize(vT) * muK * N;
                    }

                    vec3 tangentForce = gT + drive + friction;

                    float maxT = muK * N;
                    if (length(tangentForce) > maxT && length(vT) > 0.1f)
                        tangentForce = normalize(tangentForce) * maxT;
                    totalForce += tangentForce;
                    float speed = length(vT);

                    if (speed > 0.01f) {
                        vec3 rollAxis = normalize(cross(n, vT));
                        //Torque based on speed (v = w * r  ->  w = v / r)
                        float radius = snail->radius;
                        vec3 idealOmega = rollAxis * (speed / radius);
                        vec3 torque = (idealOmega - snail->w) * 10.0f;
                        totalTorque += torque;
                        
                    }
                    float groundRollingResistance = 3.5f;
                    totalTorque -= snail->w * groundRollingResistance;
                    
                }

                
                f[0] = totalForce.x;
                f[1] = totalForce.y;
                f[2] = totalForce.z;

                f[3] = totalTorque.x;
                f[4] = totalTorque.y;
                f[5] = totalTorque.z;
                return f;
            };


        snail->isSprinting = false;
		snail->isMoving = false;
        if (isGrounded && snail->retractCurrent == 0.0f) {
            float turnSpeed = radians(100.0f) * dt; 
            float moveSpeed;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && snail->stamina > 0){
                moveSpeed = snail->maxSpeed;
                snail->isSprinting = true;
            }
            else  moveSpeed = snail->moveSpeed;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                snail->v -= snailForward * moveSpeed;
                snail->isMoving = true;
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                quat turn = angleAxis(-turnSpeed, vec3(0, 1, 0));
                snail->q = normalize(snail->q * turn);
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                quat turn = angleAxis(turnSpeed, vec3(0, 1, 0));
                snail->q = normalize(snail->q * turn);
            }
            snail->P = snail->v * snail->m;

            //eating events
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                if (redFlower->checkSnailCollision(snail)) {
					snail->maxSpeed += 10.0f;
					snail->moveSpeed += 2.0f;
                
                }
                if (purpulFlower->checkSnailCollision(snail)) {
					snail->staminaMax += 100.0f;
					snail->staminaRepletionRate += 10.0f;
                    snail->staminaDepletionRate -= 10.0f;
                }

                if (mushroom->checkSnailCollision(snail)) {
                    snail->s *= 2;
                    snail->radius *= 2;
					snail->m *= 2;
				}

                if (mushroom2->checkSnailCollision(snail)) {
                    snail->s /= 2;
                    snail->radius /= 2;
                    snail->m /= 2;
                }

                if (pizza->checkSnailCollision(snail)) {
					snail->abilityUnlocked = true;
				}
            }

        }

        // Apply physics step
        snail->update(t, dt);

        // --- 5. Rendering ---
        depth_pass(); // Use light matrices implicitly

        mat4 projectionMatrix = camera->projectionMatrix;
        mat4 viewMatrix = camera->viewMatrix;
        lighting_pass(viewMatrix, projectionMatrix, snail->retractCurrent);
        //Render Skybox last
        drawSkybox(viewMatrix, projectionMatrix);
        drawStaminaBar(snail->stamina,snail->staminaMax);
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
    // Set the mouse at the center of the screen
    glfwPollEvents();
    
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
    glUniform1f(lightPowerLocation, 1000.0f);
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
        menuLoop();
        if (currentState == GAME_STATE) {
            mainLoop();
            free();
        }
    }
    catch (exception& ex) {
        cout << ex.what() << endl;
        getchar();
        free();
        return -1;
    }

    return 0;
}