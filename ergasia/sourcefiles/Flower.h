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
	//flower colors for each instance
    std::vector<glm::vec3> instanceColors;
    GLuint colorVBO;

    std::vector<bool> edible;
    // We store the color here instead of a texture ID
    glm::vec3 color;
    bool hasTexture;
    int vertexCount;
    int instanceCount;

	// ObjPath, MtlPath, Terrain, Count, Scale, UseMtl, HasTexture
    Flower(const char* objPath, const char* mtlPath, Heightmap* terrain, int count, float scale, bool mtl);
    ~Flower();

    void draw(GLuint shaderProgram,bool drawShading);
    bool checkSnailCollision(Snail* snail);
    bool checkSnailCollisionNotRetracted(Snail* snail);
private:
    void loadMTL(const char* path);
    void generatePositions(Heightmap* terrain, int count, float scale);
};