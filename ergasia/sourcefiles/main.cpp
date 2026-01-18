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
#include "Flower.h"
#include "Collision.h"
#include <common/model.h>
#include <common/texture.h>
#include <stb_image_aug.h>
#include "Eagle.h"
#include "Menu.h"
#include "Tree.h"

#ifdef _WIN32
#include <windows.h>
extern "C" {
    // Για κάρτες NVIDIA
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;

    // Για κάρτες AMD
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

using namespace std;
using namespace glm;

void initialize();
void createContext();
void mainLoop();
void free();
struct Light; struct Material;
void uploadMaterial(const Material& mtl);
void uploadLight(const Light& light);
void uploadSnailMaterial(const Material& mtl);
void uploadSnailLight(const Light& light);
void uploadTreeLight(const Light& light);

#define W_WIDTH 1920
#define W_HEIGHT 1080
#define TITLE "Super Snail"
#define SHADOW_WIDTH 2048
#define SHADOW_HEIGHT 2048
#define MAP_SIZE 2000
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

// Trees and flowers
GLuint treeLaLocation, treeLdLocation, treeLsLocation, treeLightPositionLocation, treeLightPowerLocation, treeDepthMapSampler;

// Shadow Pass Uniforms
GLuint shadowViewProjectionLocation;
GLuint shadowModelLocation;

GLuint terrainGrassTexture, terrainRockTexture, terrainRubberTexture, snailDiffuseTexture;
GLuint depthFBO, depthTexture;
//skybox
GLuint skyProjectionMatrixLocation, skyViewMatrixLocation;
GLuint skyboxVAO, skyboxVBO;
GLuint cubemapTexture;

//stamina bar
GLuint staminaShader;
GLint modelLoc, colorLoc,useTextureMenuLoc;  

//instanced rendering
Tree oakTree;
Tree pineTree;
std::vector<glm::mat4> allTreeMatrices;
//grass
Tree grassSystem;

//LOULOUDIAAAAAA (me powerups)
Flower* redFlower,* purpulFlower, * pizza, *mushroom, *mushroom2;

//
GLuint treeIconTex;
GLuint flowerIconTex;

//menu
enum GameState {MENU_STATE, GAME_STATE, SETTINGS_STATE};
GameState currentState;
Menu* mainMenu;
int desiredTreeCount = 700;   
int desiredFlowerCount = 100;
struct Material {
    vec4 Ka; 
    vec4 Kd;
    vec4 Ks; 
    float Ns;     
    string map_Kd; 
    GLuint textureID;   
};

//eagly
Eagle* eagle;
GLuint eagleIconTex;

//collision detection
unordered_map<GridKey, vector<int>, GridKeyHash> treeGrid;
float cellSize = 10.0f;

struct FlowerHandle {
    Flower* type; 
    int index;   
};

unordered_map<GridKey, vector<FlowerHandle>, GridKeyHash> flowerGrid;

Light* light = new Light(window,
    vec4{ 1, 1, 1, 1 },
    vec4{ 1, 1, 1, 1 },
    vec4{ 1, 1, 1, 1 },
    vec3(0, 200, 0)
);

Heightmap* terrain;
Snail* snail;
Drawable* quad;


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

void buildCollisionGrid(const std::vector<mat4>& instanceMatrices) {
    treeGrid.clear();
    for (int i = 0; i < instanceMatrices.size(); i++) {
        vec3 pos = vec3(instanceMatrices[i][3]);

        int gridX = static_cast<int>(floor(pos.x / cellSize));
        int gridZ = static_cast<int>(floor(pos.z / cellSize));

        treeGrid[{gridX, gridZ}].push_back(i);
    }
}

void buildFlowerGrid() {
    flowerGrid.clear();

    // List of all your flower types
    vector<Flower*> allFlowers = { redFlower, purpulFlower, mushroom, mushroom2, pizza };

    for (Flower* flower : allFlowers) {
        for (int i = 0; i < flower->instanceMatrices.size(); i++) {
            vec3 pos = vec3(flower->instanceMatrices[i][3]);

            int gridX = static_cast<int>(floor(pos.x / cellSize));
            int gridZ = static_cast<int>(floor(pos.z / cellSize));

            // Store the pointer and the index
            flowerGrid[{gridX, gridZ}].push_back({ flower, i });
        }
    }
}

vector<mat4> generateGrassPositions(int amount) {
    vector<mat4> matrices;
    int attempts = 0;

    while (matrices.size() < amount && attempts < amount * 2) {
        attempts++;

        // Random Position
        float x = (rand() % (MAP_SIZE * 2) - MAP_SIZE);
        float z = (rand() % (MAP_SIZE * 2) - MAP_SIZE);
        float y = terrain->getHeightAt(x, z);

        
        float type = terrain->getGroundTypeAt(x, z);
        if (abs(type) > 0.2f ) continue; // spawn only on grass

        mat4 model = translate(mat4(1.0f), vec3(x, y, z));

        vec3 normal = normalize(terrain->getNormalAt(x, z));
        vec3 up = vec3(0.0f, 1.0f, 0.0f);

        if (abs(dot(up, normal)) < 0.999f) { 
            vec3 axis = normalize(cross(up, normal));
            float angle = acos(dot(up, normal));
            model = rotate(model, angle, axis);
        }
        model = rotate(model, radians((float)(rand() % 360)), vec3(0, 1, 0));

        float scaleVal = 30.0f;
        model = scale(model, vec3(scaleVal, scaleVal/2, scaleVal));

        matrices.push_back(model);
    }
    return matrices;
}

vector<mat4> generateTreePositions(int amount,float scalar) {
	vector<mat4> instanceMatrices;
    for (int i = 0; i < amount; i++) {
        float x = (rand() % (MAP_SIZE * 2) - MAP_SIZE); // Random X
        float z = (rand() % (MAP_SIZE * 2) - MAP_SIZE); // Random Z
        float y = terrain->getHeightAt(x, z);    // Get Y from heightmap
        mat4 model = translate(mat4(1.0f), vec3(x, y, z));
        model = rotate(model, radians((float)(rand() % 360)), vec3(0, 1, 0)); // Random rotation
        model = scale(model, vec3(scalar, scalar, scalar));
        instanceMatrices.push_back(model);
    }
	return instanceMatrices;
}

void initTree() {
    allTreeMatrices.clear(); 

    oakTree.init("models/tree.obj", "models/tree2.bmp");
    vector<mat4> oakPos = generateTreePositions(desiredTreeCount/2,4.0f);
    oakTree.setupInstances(oakPos);

    allTreeMatrices.insert(allTreeMatrices.end(), oakPos.begin(), oakPos.end());


    pineTree.init("models/tree2.obj", "models/tree2.bmp");
    vector<mat4> pinePos = generateTreePositions(desiredTreeCount/2,0.1f);
    pineTree.setupInstances(pinePos);

    // Create tree grid
    allTreeMatrices.insert(allTreeMatrices.end(), pinePos.begin(), pinePos.end());
    grassSystem.init("models/grass2.obj","textures/grass3.bmp");
    vector<mat4> grassPos = generateGrassPositions(500);
    grassSystem.setupInstances(grassPos);

    buildCollisionGrid(allTreeMatrices);
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
	snailUseTextureLocation = glGetUniformLocation(snailShaderProgram, "useTexture");
    //Trees light location
    treeLaLocation = glGetUniformLocation(vegetShader,"light.La"); 
    treeLdLocation = glGetUniformLocation(vegetShader, "light.Ld"); 
    treeLsLocation = glGetUniformLocation(vegetShader, "light.Ls"); 
    treeLightPositionLocation = glGetUniformLocation(vegetShader, "light.lightPosition_worldspace");  
    treeDepthMapSampler = glGetUniformLocation(vegetShader, "shadowMapSampler");

    //Shadow Loader Uniforms 
    shadowViewProjectionLocation = glGetUniformLocation(shadowLoader, "VP");
    shadowModelLocation = glGetUniformLocation(shadowLoader, "M");
	//skybox uniforms
	skyViewMatrixLocation = glGetUniformLocation(skyboxShader, "view");
	skyProjectionMatrixLocation = glGetUniformLocation(skyboxShader, "projection");
    modelLoc = glGetUniformLocation(staminaShader, "model");
    colorLoc = glGetUniformLocation(staminaShader, "barColor");
    useTextureMenuLoc = glGetUniformLocation(staminaShader, "useTexture");

	// Terrain Initialization
	terrainGrassTexture = loadSOIL("textures/grass3.bmp");
    terrainRockTexture = loadSOIL("textures/rockyGrass2.bmp");
    terrainRubberTexture = loadSOIL("textures/rubber.bmp");
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

    initUI();

	//menu init
    mainMenu = new Menu();
    mainMenu->init(W_WIDTH, W_HEIGHT);
    mainMenu->initText("textures/numbers.png");

    treeIconTex = loadSOIL("textures/lowPolyTree.bmp"); // Reuse tree texture or use a specific icon
    flowerIconTex = loadSOIL("textures/lowPolyRose.bmp");
	eagleIconTex = loadSOIL("textures/eagle.bmp");

}

void updateProgressBar(float percent) {
    // 1. SETUP & CLEAR
    glDisable(GL_DEPTH_TEST);
    glUseProgram(staminaShader);
    glClear(GL_COLOR_BUFFER_BIT); // Necessary to remove artifacts

    // 2. DRAW BACKGROUND (MENU) - Uses Pixel Coordinates (0 to 1024)
    int page = 0;
    if (currentState == SETTINGS_STATE) page = 1;

    // This call internally sets the 'projection' uniform to Ortho(0, 1024...)
    mainMenu->draw(staminaShader, W_WIDTH, W_HEIGHT, page);

    if (currentState == SETTINGS_STATE) {
        mainMenu->drawNumber(staminaShader, desiredTreeCount, vec2(W_WIDTH * 0.59f, W_HEIGHT * 0.75f), 1.0f, W_WIDTH, W_HEIGHT);
        mainMenu->drawNumber(staminaShader, desiredFlowerCount, vec2(W_WIDTH * 0.59f, W_HEIGHT * 0.50f), 1.0f, W_WIDTH, W_HEIGHT);

        mainMenu->drawIcon(staminaShader, treeIconTex, vec2(W_WIDTH * 0.50f, W_HEIGHT * 0.75f), vec2(100, 100));
        mainMenu->drawIcon(staminaShader, flowerIconTex, vec2(W_WIDTH * 0.50f, W_HEIGHT * 0.50f), vec2(100, 100));
    }

    mat4 identity = mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(staminaShader, "projection"), 1, GL_FALSE, &identity[0][0]);

    quad->bind();

    float startX = -1.0f; // Start slightly left of center
    float startY = -1.0f; // Lower half of screen
    float totalWidth = 2.0f; // Width (2.0 is full screen)
    float height = 0.1f;    // Thickness

    // --- Draw Grey Background ---
    mat4 bgModel = translate(mat4(1.0f), vec3(startX, startY, 0.0f));
    bgModel = scale(bgModel, vec3(totalWidth, height, 1.0f));

    glUniform1i(useTextureMenuLoc, 0); // Solid Color
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &bgModel[0][0]);
    glUniform3f(colorLoc, 0.3f, 0.3f, 0.3f); // Dark Grey
    quad->draw();

    // --- Draw Green Fill ---
    mat4 fgModel = translate(mat4(1.0f), vec3(startX, startY, 0.0f));
    fgModel = scale(fgModel, vec3(totalWidth * (percent / 100.0f), height, 1.0f));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &fgModel[0][0]);
    glUniform3f(colorLoc, 0.0f, 1.0f, 0.0f); // Green
    quad->draw();

    // 5. UPDATE SCREEN
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void createContext2() {
    // 0% - Start
    updateProgressBar(0.0f);

    // Terrain 
	//rows, columns, numHills, minRadius, maxRadius, minHeight, maxHeight, scalar, scalarY
    Heightmap::HillAlgorithmParameters params(400, 400, 100, 10, 40, -2.0f, 5.0f, MAP_SIZE * 2, 50);
    terrain = new Heightmap(params);

    // 20% - Terrain Done
    updateProgressBar(20.0f);

    // Snail Initialization 
    float spawnX = 0.0f;
    float spawnZ = 0.0f;
    float spawnY = terrain->getHeightAt(spawnX, spawnZ);
    vec3 initPos = vec3(spawnX, spawnY + 1.0f, spawnZ);
    snail = new Snail(initPos, 1.0f, 1.2f);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 40% - Snail Done
    updateProgressBar(40.0f);

    // Skybox
    initSkybox();

    // 60% - Skybox Done
    updateProgressBar(60.0f);

    initTree();

    // 80% - Geometry Done
    updateProgressBar(80.0f);

    redFlower = new Flower("models/flowers/redFlower.obj", "models/flowers/redFlower.mtl", terrain, desiredFlowerCount, 5.0f, true, MAP_SIZE);
 
    updateProgressBar(85.0f);

    purpulFlower = new Flower("models/flowers/bellFlower.obj", "models/flowers/bellFlower.mtl", terrain, desiredFlowerCount, 4.0f, true, MAP_SIZE);
    mushroom = new Flower("models/flowers/mushroom.obj", "models/flowers/mushroom.mtl", terrain, desiredFlowerCount / 2, 0.3f, true, MAP_SIZE);
   
    updateProgressBar(90.0f);

    mushroom2 = new Flower("models/flowers/mushroom.obj", "models/flowers/mushroom2.mtl", terrain, desiredFlowerCount/2, 0.3f, true, MAP_SIZE);
    pizza = new Flower("models/flowers/pizza.obj", "models/flowers/pizza.bmp", terrain, 1, 1.0f, false, 40);

    buildFlowerGrid();

    eagle = new Eagle(vec3(0, 300, 0));
    // 100% - Finished!
    updateProgressBar(100.0f);
}

void drawTerrain(GLuint program, GLuint modelLocation, GLuint diffuseSampler) {
    glUseProgram(program);

    uploadLight(*light); 

    mat4 modelMatrix = terrain->returnplaneMatrix();
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, terrainGrassTexture);
    glUniform1i(diffuseSampler, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, terrainRockTexture);
    glUniform1i(glGetUniformLocation(program, "detailSampler"), 1);
    glUniform1i(useTextureLocation, 1); 

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, terrain->splatTextureID);
    glUniform1i(glGetUniformLocation(program, "splatMapSampler"), 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, terrainRubberTexture);
    glUniform1i(glGetUniformLocation(program, "bouncySampler"), 4);

    glUniform1i(useTextureLocation, 1);
    terrain->bind();
    terrain->draw();

    terrain->bind();
    terrain->draw();
}

void drawSnail(GLuint program, GLuint modelLocation, float time, float speed, float retractFactor) {
    glUseProgram(program);

    uploadSnailLight(*light); 
    
    glUniform1f(snailTimeLocation, time);
    glUniform1f(snailSpeedLocation, speed);
    glUniform1f(snailRetractFactorLocation, retractFactor);


    mat4 modelMatrix = snail->snailModelMatrix;
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
    glUniform1f(snailUseTextureLocation, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, snailDiffuseTexture);
    glUniform1i(snailColorSampler, 0); 

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(snailDepthMapSampler, 2);


    mat4 lightVP = light->lightVP();
    glUniformMatrix4fv(snailLightVPLocation, 1, GL_FALSE, &lightVP[0][0]);
	
    snail->draw();
}

void drawSkybox(mat4 view, mat4 projectionMatrix) {
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxShader);

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
    //DRAW BACKGROUND 
    
    mat4 bgModel = translateMat * scale(mat4(1.0f), vec3(1.0f/ 2.0f,1.0/ 20.0f, 1.0f));
    

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &bgModel[0][0]);
    glUniform3f(colorLoc, 0.2f, 0.2f, 0.2f);
    quad->draw();

    //DRAW FILL 
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
    glUseProgram(shadowLoader);


    glUniform1i(glGetUniformLocation(shadowLoader, "isInstanced"), 0);
    mat4 lightVP = light->lightVP();
    glUniformMatrix4fv(shadowViewProjectionLocation, 1, GL_FALSE, &lightVP[0][0]);
    drawTerrain(shadowLoader, shadowModelLocation, diffuseColorSampler); 
    mat4 snailModelMatrix = snail->snailModelMatrix;
    glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &snailModelMatrix[0][0]);
    snail->draw();

    glUniform1i(glGetUniformLocation(shadowLoader, "isInstanced"), 1);
    oakTree.draw(shadowLoader);
    pineTree.draw(shadowLoader);
    grassSystem.draw(shadowLoader);
    drawFlowers(shadowLoader, true);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void lighting_pass(mat4 viewMatrix, mat4 projectionMatrix, float retractFactor,float t) {
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

    //draw eagle
    glUniform1i(useTextureLocation, 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, eagleIconTex);
	eagle->draw(terrainProgram, modelMatrixLocation, diffuseColorSampler);

    //draw Snail
    glUseProgram(snailShaderProgram);
    glUniformMatrix4fv(snailViewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(snailProjectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);    
    float snailSpeed = length(snail->v);
    drawSnail(snailShaderProgram, snailModelMatrixLocation, (float)glfwGetTime(), snailSpeed, retractFactor);

    glUseProgram(vegetShader);
    glUniformMatrix4fv(glGetUniformLocation(vegetShader, "P"), 1, GL_FALSE, &projectionMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(vegetShader, "V"), 1, GL_FALSE, &viewMatrix[0][0]);
    glUniform1f(glGetUniformLocation(vegetShader, "time"), t);
    glUniformMatrix4fv(glGetUniformLocation(vegetShader, "lightVP"), 1, GL_FALSE, &lightVP[0][0]);
    uploadTreeLight(*light);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(treeDepthMapSampler, 2);
    oakTree.draw(vegetShader);
    pineTree.draw(vegetShader);

    grassSystem.draw(vegetShader);


    glUseProgram(flowerShading);
    glUniformMatrix4fv(glGetUniformLocation(flowerShading, "P"), 1, GL_FALSE, &projectionMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(flowerShading, "V"), 1, GL_FALSE, &viewMatrix[0][0]);
    drawFlowers(flowerShading, false);

    glBindVertexArray(0);

}

void drawSpeedBar(float speed, float maxSpeed) {
    glDisable(GL_DEPTH_TEST);
    glUseProgram(staminaShader);

    float percentage = clamp(speed/maxSpeed , 0.0f, 1.0f);

    mat4 translateMat = translate(mat4(1.0f), vec3(0.5f, -0.75f, 0.0f));

    quad->bind();

    mat4 bgModel = translateMat * scale(mat4(1.0f), vec3(0.4f, 0.05f, 1.0f));
    glUniform1i(useTextureMenuLoc, 0); 
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &bgModel[0][0]);
    glUniform3f(colorLoc, 0.2f, 0.2f, 0.2f); 
    quad->draw();

    mat4 fgModel = translateMat * scale(mat4(1.0f), vec3(0.4f * percentage, 0.05f, 1.0f));

    vec3 speedColor = vec3(0.0f, 0.5f + (0.5f * percentage), 1.0f);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &fgModel[0][0]);
    glUniform3f(colorLoc, speedColor.r, speedColor.g, speedColor.b);
    quad->draw();
   
    vec2 textPos = vec2(0.75*W_WIDTH, 0.05 * W_HEIGHT);

    mainMenu->drawNumber(staminaShader, (int)speed, textPos, 1.0f, W_WIDTH, W_HEIGHT);

    glEnable(GL_DEPTH_TEST);
}

void free() {
    delete terrain;
    glDeleteProgram(terrainProgram);
    glDeleteProgram(snailShaderProgram); // Cleanup new shader
    glDeleteProgram(shadowLoader);
    glfwTerminate();
}

void tryEatFlowers() {
    int snailGridX = static_cast<int>(floor(snail->x.x / cellSize));
    int snailGridZ = static_cast<int>(floor(snail->x.z / cellSize));

    // Check 3x3 area
    for (int x = -1; x <= 1; x++) {
        for (int z = -1; z <= 1; z++) {
            GridKey key = { snailGridX + x, snailGridZ + z };

            if (flowerGrid.count(key)) {
                for (const auto& handle : flowerGrid[key]) {
                    // Pass 'false' for isRetracted to trigger Eating Logic
                    bool ate = handle.type->checkCollisionByIndex(handle.index, snail, false);

                    if (ate) {
                        if (handle.type == redFlower) { snail->maxSpeed += 10.0f; snail->moveSpeed += 2.0f; }
                        else if (handle.type == purpulFlower) { snail->staminaMax += 100.0f; snail->staminaDepletionRate -= 10.0f; }
                        else if (handle.type == mushroom) {
                            if (snail->s <5.0f) { snail->s *= 2; snail->radius *= 2; snail->m *= 2; }
                        }
                        else if (handle.type == mushroom2) { snail->s /= 2; snail->radius /= 2; snail->m /= 2; }
                        else if (handle.type == pizza) { snail->abilityUnlocked = true; }
                        return; 
                    }
                }
            }
        }
    }
}

void applyFlowerPhysics() {
    int snailGridX = static_cast<int>(floor(snail->x.x / cellSize));
    int snailGridZ = static_cast<int>(floor(snail->x.z / cellSize));

    for (int x = -1; x <= 1; x++) {
        for (int z = -1; z <= 1; z++) {
            GridKey key = { snailGridX + x, snailGridZ + z };

            if (flowerGrid.count(key)) {
                for (const auto& handle : flowerGrid[key]) {
                    // Pass 'true' for isRetracted to trigger Physics Logic
                    handle.type->checkCollisionByIndex(handle.index, snail, true);
                }
            }
        }
    }
}

void menuLoop() {
    double x, y;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Setup for 2D rendering
    glUseProgram(staminaShader);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int activePage = 0;
    do {
        if (currentState == MENU_STATE) activePage = 0;
        else if (currentState == SETTINGS_STATE) activePage = 1;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mainMenu->draw(staminaShader, W_WIDTH, W_HEIGHT, activePage);

        if (currentState == SETTINGS_STATE) {

            mainMenu->drawNumber(staminaShader, desiredTreeCount, vec2(W_WIDTH * 0.59f, W_HEIGHT * 0.75f), 1.0f, W_WIDTH, W_HEIGHT);
            mainMenu->drawNumber(staminaShader, desiredFlowerCount, vec2(W_WIDTH * 0.59f, W_HEIGHT * 0.50f), 1.0f, W_WIDTH,W_HEIGHT);

            mainMenu->drawIcon(staminaShader, treeIconTex, vec2(W_WIDTH * 0.50f, W_HEIGHT * 0.75f), glm::vec2(100, 100));
            mainMenu->drawIcon(staminaShader, flowerIconTex, vec2(W_WIDTH * 0.50f, W_HEIGHT * 0.50f), glm::vec2(100, 100));
        }
        
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            double x, y;
            glfwGetCursorPos(window, &x, &y);

            int action = mainMenu->checkClick(x, y, W_HEIGHT, activePage);

            if (action == 3) { // Clicked on "Settings"
                currentState = SETTINGS_STATE; 
                glfwWaitEventsTimeout(0.2); 
            }
			else if (action == 1) { // Clicked on "Start Game" 
                createContext2();
                mainLoop(); // Generate world
                currentState = GAME_STATE; // Start Game
            }
            else if (action == 2) glfwSetWindowShouldClose(window, 1); // Exit
            else if (action == 4) { 
                desiredTreeCount += 10;
                glfwWaitEventsTimeout(0.2);
            } 
            else if (action == 5) {
                desiredTreeCount -= 10;
                glfwWaitEventsTimeout(0.2);
            }
            else if (action == 6) {
                desiredFlowerCount += 5;
				glfwWaitEventsTimeout(0.2);
            }else if (action == 7){
				desiredFlowerCount -= 5;
				glfwWaitEventsTimeout(0.2);
            }


        }
            glfwSwapBuffers(window);
            
            glfwPollEvents();
        } while ((currentState == MENU_STATE || currentState == SETTINGS_STATE)  && glfwWindowShouldClose(window) == 0 && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS);
    free();
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
        eagle->update(dt, snail);
        bool controlPressed = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);

        if (controlPressed && !isControlKeyHeld){
            
            if (snail->retractTarget == 0.0f) {
                snail->retractTarget = 1.0f;
                snail->isSprinting = false;
            }
            else {
                snail->retractTarget = 0.0f;
            }


        }
        isControlKeyHeld = controlPressed;
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
        else if (snail->retractCurrent > 0.0f) {
            snail->isRetracted = true;
		}

        if (snail->retractCurrent > 0.0f)applyFlowerPhysics();
        onTree = handleSnailTreeCollision(snail, allTreeMatrices);
        isGrounded = handleSnailTerrainCollision(snail, terrain, onTree);
        if (isGrounded) {
            vec3 n = normalize(terrain->getNormalAt(snail->x.x, snail->x.z));

            float impactSpeed = dot(snail->v, n);
            float groundType = terrain->getGroundTypeAt(snail->x.x, snail->x.z);

            float bounciness = 0.0f; 

            if (groundType < -0.5f) {
                bounciness = 1.2f; 
            }

            if (impactSpeed < 0.0f)
                snail->v -= impactSpeed * n * (1.0f + bounciness);

            snail->P = snail->m * snail->v;

        }
        vec3 snailForward = snail->q * vec3(0, 0, -1); //-Z is forward
        vec3 snailRight = snail->q * vec3(1, 0, 0);

        snail->forcing = [&](float t, const vector<float>& y) -> vector<float>
            {
                vector<float> f(6, 0.0f);
                if (!snail->isRetracted) {
                    float stopDamping = 7.0f; // Very strong braking

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

                
                vec3 gravity(0.0f, -snail->m * g, 0.0f);
                vec3 totalForce(0.0f);
                if (!(eagle->state == GRABBING))
                    totalForce = gravity;
                vec3 totalTorque(0.0f);

                if (isGrounded)
                {
                    const float inputForce = 800.0f; 
                    const float BIAS = 1e-4f;

                    float groundType = abs(terrain->getGroundTypeAt(snail->x.x, snail->x.z)) + 0.2f;

                    float muK = 2.0f * groundType;
                    float muS = 5.0f * groundType;

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
                    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && snail->abilityUnlocked) totalForce += 50.0f * -gravity;
                    vec3 friction(0.0f);
                    if (length(vT) < 0.05f && length(drive + gT) < muS * N)
                    {
                        // STATIC friction  cancel the desire to move
                        friction = -(0.5f * drive + gT);
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
                        float radius = snail->radius;
                        vec3 idealOmega = rollAxis * (speed / radius);
                        vec3 torque = (idealOmega - snail->w) * 10.0f;
                        totalTorque += torque;
                        
                    }

                    float groundRollingResistance = mix(5.5f, 2.5f, groundType);
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


        
        if (isGrounded && snail->retractCurrent == 0.0f) {
            snail->isSprinting = false;
            snail->isMoving = false;
            float turnSpeed = radians(100.0f) * dt; 
            float moveSpeed;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && snail->stamina > 0){
                moveSpeed = snail->maxSpeed;
                snail->isSprinting = true;
            }
            else  moveSpeed = snail->moveSpeed;
            vec3 n;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                snail->v -= snailForward * moveSpeed;
                snail->isMoving = true;
            }
            else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                snail->v += snailForward * moveSpeed / 10.0f;
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
                tryEatFlowers();
            }

        }
        handleBoxSnailCollision(terrain, snail);

        snail->update(t, dt);

        depth_pass(); 

        mat4 projectionMatrix = camera->projectionMatrix;
        mat4 viewMatrix = camera->viewMatrix;
        lighting_pass(viewMatrix, projectionMatrix, snail->retractCurrent, currentTime);
        //Render Skybox last
        drawSkybox(viewMatrix, projectionMatrix);
        drawStaminaBar(snail->stamina,snail->staminaMax);
        drawSpeedBar(length(vec3(snail->v.x,0, snail->v.z )), 200.0f);
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
    /*const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    window = glfwCreateWindow(mode->width, mode->height, TITLE, glfwGetPrimaryMonitor(), NULL);
    */
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
    glUniform4f(KaLocation, mtl.Ka.r, mtl.Ka.g, mtl.Ka.b, mtl.Ka.a); 
    glUniform4f(KdLocation, mtl.Kd.r, mtl.Kd.g, mtl.Kd.b, mtl.Kd.a);
    glUniform4f(KsLocation, mtl.Ks.r, mtl.Ks.g, mtl.Ks.b, mtl.Ks.a);
    glUniform1f(NsLocation, mtl.Ns);
}

void uploadSnailMaterial(const Material& mtl) {
    glUniform4f(snailKaLocation, mtl.Ka.r, mtl.Ka.g, mtl.Ka.b, mtl.Ka.a); 
    glUniform4f(snailKdLocation, mtl.Kd.r, mtl.Kd.g, mtl.Kd.b, mtl.Kd.a);
    glUniform4f(snailKsLocation, mtl.Ks.r, mtl.Ks.g, mtl.Ks.b, mtl.Ks.a);
    glUniform1f(snailNsLocation, mtl.Ns);
}

void uploadLight(const Light& light) {
    glUniform4f(LaLocation, light.La.r, light.La.g, light.La.b, light.La.a); 
    glUniform4f(LdLocation, light.Ld.r, light.Ld.g, light.Ld.b, light.Ld.a);
    glUniform4f(LsLocation, light.Ls.r, light.Ls.g, light.Ls.b, light.Ls.a);
    glUniform3f(lightPositionLocation, light.lightPosition_worldspace.x,
        light.lightPosition_worldspace.y, light.lightPosition_worldspace.z);
    glUniform1f(lightPowerLocation, 1000.0f);
}

void uploadSnailLight(const Light& light) {
    glUniform4f(snailLaLocation, light.La.r, light.La.g, light.La.b, light.La.a); 
    glUniform4f(snailLdLocation, light.Ld.r, light.Ld.g, light.Ld.b, light.Ld.a);
    glUniform4f(snailLsLocation, light.Ls.r, light.Ls.g, light.Ls.b, light.Ls.a);
    glUniform3f(snailLightPositionLocation, light.lightPosition_worldspace.x,
        light.lightPosition_worldspace.y, light.lightPosition_worldspace.z);
    glUniform1f(snailLightPowerLocation, 100.0f);
}

void uploadTreeLight(const Light& light) {
    glUniform4f(treeLaLocation, light.La.r, light.La.g, light.La.b, light.La.a);
    glUniform4f(treeLdLocation, light.Ld.r, light.Ld.g, light.Ld.b, light.Ld.a);
    glUniform4f(treeLsLocation, light.Ls.r, light.Ls.g, light.Ls.b, light.Ls.a);
    glUniform3f(treeLightPositionLocation, light.lightPosition_worldspace.x,
        light.lightPosition_worldspace.y, light.lightPosition_worldspace.z);
}

int main(void) {
    try {
        initialize();
        createContext();
        menuLoop();
    }
    catch (exception& ex) {
        cout << ex.what() << endl;
        getchar();
        free();
        return -1;
    }

    return 0;
}