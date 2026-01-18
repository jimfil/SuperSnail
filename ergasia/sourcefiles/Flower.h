#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "heightmap.h"
#include "Snail.h"

class Flower {
public:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<glm::mat4> instanceMatrices;

    GLuint VAO, vertexVBO, uvVBO, normalVBO, instanceVBO;
    GLuint textureID;

    std::vector<glm::vec3> instanceColors;
    GLuint colorVBO;

    std::vector<bool> edible;

    glm::vec3 color;
    bool hasTexture;
    int vertexCount;
    int instanceCount;

    Flower(const char* objPath, const char* mtlPath, Heightmap* terrain, int count, float scale, bool mtl, int mapSize);
    ~Flower();

    void draw(GLuint shaderProgram,bool drawShading);
    bool checkCollisionByIndex(int index, Snail* snail, bool isRetracted);
private:
    void loadMTL(const char* path);
    void generatePositions(Heightmap* terrain, int count, float scale, int mapSize);
};