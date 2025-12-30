#include "Flower.h"
#include <common/model.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>
#include <common/texture.h>

using namespace std;
using namespace glm;

void Flower::loadMTL(const char* path) {
    // Default to Red if file fails
    this->color = vec3();

    ifstream file(path);
    if (!file.is_open()) {
        cout << "Error: Could not open MTL file " << path << endl;
        return;
    }

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string key;
        ss >> key;

        // Look for Diffuse Color
        if (key == "Kd") {
            ss >> color.r >> color.g >> color.b;
            break;
        }
    }

}

void Flower::generatePositions(Heightmap* terrain, int count, float scale) {
    instanceMatrices.clear();
    for (int i = 0; i < count; i++) {
        float x = (rand() % 100 );
        float z = (rand() % 100 );
        float y = terrain->getHeightAt(x, z);

        mat4 model = translate(mat4(1.0f), vec3(x, y, z));
        model = rotate(model, radians((float)(rand() % 360)), vec3(0, 1, 0));
        model = glm::scale(model, vec3(scale));
        instanceMatrices.push_back(model);
    }
}

Flower::Flower(const char* objPath, const char* mtlPath, Heightmap* terrain, int count, float scale, bool mtl) {
    this->instanceCount = count;
	this->hasTexture = !mtl;
    // 1. Load Color from MTL
    if (mtl) loadMTL(mtlPath);
    else textureID = loadSOIL(mtlPath);

    // 2. Load Geometry
    loadOBJWithTiny(objPath, vertices, uvs, normals);
    if (vertices.empty()) {
        cout << "CRITICAL ERROR: Failed to load model or model is empty: " << objPath << endl;
        return; // Stop here so we don't crash at &vertices[0]
    }
    vertexCount = vertices.size();

    // 3. Generate Positions
    generatePositions(terrain, count, scale);
    instanceColors.resize(count, this->color);
	edible.resize(count, true);
    // 4. Setup OpenGL Buffers
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Vertices
    glGenBuffers(1, &vertexVBO);
    glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    if (!uvs.empty()) {
        glGenBuffers(1, &uvVBO);
        glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
        glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), &uvs[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    // Normals
    if (normals.empty()) {
        // Generate dummy Up normals if missing
        for (size_t i = 0; i < vertices.size(); i++) normals.push_back(vec3(0, 1, 0));
    }
    glGenBuffers(1, &normalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), &normals[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    // Instances
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(mat4), &instanceMatrices[0], GL_STATIC_DRAW);

    // Matrix Attributes (3, 4, 5, 6)
    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(i * sizeof(vec4)));
        glVertexAttribDivisor(3 + i, 1);
    }

    glGenBuffers(1, &colorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    // Use DYNAMIC_DRAW because we will update this often!
    glBufferData(GL_ARRAY_BUFFER, instanceColors.size() * sizeof(vec3), &instanceColors[0], GL_DYNAMIC_DRAW);

    // Bind to VAO
    glBindVertexArray(VAO); // Make sure VAO is bound!
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glVertexAttribDivisor(7, 1);

    glBindVertexArray(0);
}

Flower::~Flower() {
    glDeleteBuffers(1, &vertexVBO);
    glDeleteBuffers(1, &uvVBO);
    glDeleteBuffers(1, &normalVBO);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteTextures(1, &textureID);
}

void Flower::draw(GLuint shaderProgram,bool drawShading) {
    // 1. Disable Texture usage in shader
    if (!drawShading) {
        if (!this->hasTexture) {
            glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);


            glUniform4f(glGetUniformLocation(shaderProgram, "mtl.Kd"), color.r, color.g, color.b, 1.0f);

            glUniform4f(glGetUniformLocation(shaderProgram, "mtl.Ka"), 1.0f, 1.0f, 1.0f, 1.0f);
        }
        else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);
        }
    }
    else {
        glUniform1i(glGetUniformLocation(shaderProgram, "isInstanced"), 1);
    }
    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, instanceCount);
    glBindVertexArray(0);
}



bool Flower::checkSnailCollision(Snail* snail) {

    float combinedRadius = snail->radius + 0.3f;
    float detectionDist = combinedRadius + 0.1f;
        for (int i = 0; i < instanceMatrices.size(); i++) {
            vec3 flow = vec3(instanceMatrices[i][3]); // Get Position

            float dx = snail->x.x - flow.x;
            float dz = snail->x.z - flow.z;
            float distSq = dx * dx + dz * dz;

            if (distSq < detectionDist * detectionDist && this->edible[i]) {
                if (!this->hasTexture) {
                    instanceColors[i] = vec3(this->color.x * 0.1f, this->color.y * 0.1f, this->color.z * 0.1f);
                    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
                    glBufferSubData(GL_ARRAY_BUFFER, i * sizeof(vec3), sizeof(vec3), &instanceColors[i]);
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                }
                else {
                    this->color = vec3(0.1f, 0.1f, 0.1f);
					this->hasTexture = false;
                    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
                    glBufferSubData(GL_ARRAY_BUFFER, i * sizeof(vec3), sizeof(vec3), &instanceColors[i]);
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                }
                this->edible[i] = false;
                return true;
            }
        }

    return false;
}

bool Flower::checkSnailCollisionNotRetracted(Snail* snail) {

    float combinedRadius = snail->radius + 0.3f;
    float detectionDist = combinedRadius + 0.1f;
    for (int i = 0; i < instanceMatrices.size(); i++) {
        vec3 flow = vec3(instanceMatrices[i][3]); // Get Position

        float dx = snail->x.x - flow.x;
        float dz = snail->x.z - flow.z;
        float distSq = dx * dx + dz * dz;

        if (distSq < detectionDist * detectionDist && this->edible[i]) {
            snail-> v*= 0.5f; // Slow down snail
			snail->P = snail->v * snail->m;
			snail->w *= 0.5f;
			snail->L = snail->w * snail->I_inv;
			return true;
        }
    }

    return false;
}