#ifndef TREE_H
#define TREE_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <common/model.h>
#include <common/texture.h>

class Tree {
public:
    GLuint vao, vertexVBO, uvVBO, normalVBO, instanceVBO;
    GLuint texture;
    int vertexCount;
    std::vector<glm::mat4> instanceMatrices;

    Tree() : vao(0), texture(0), vertexCount(0) {}

    void init(const std::string& objPath, const std::string& texPath) {
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> normals;

        loadOBJWithTiny(objPath.c_str(), vertices, uvs, normals);
        vertexCount = vertices.size();
        texture = loadSOIL(texPath.c_str());

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Vertices
        glGenBuffers(1, &vertexVBO);
        glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        // UVs
        glGenBuffers(1, &uvVBO);
        glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
        glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

        // Normals
        glGenBuffers(1, &normalVBO);
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        glGenBuffers(1, &instanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

        for (int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(3 + i);
            glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * sizeof(glm::vec4)));
            glVertexAttribDivisor(3 + i, 1);
        }
        glBindVertexArray(0);
    }

    void setupInstances(const std::vector<glm::mat4>& matrices) {
        instanceMatrices = matrices;
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4), &instanceMatrices[0], GL_STATIC_DRAW);
    }

    void draw(GLuint shader) {
        if (instanceMatrices.empty()) return;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, instanceMatrices.size());
        glBindVertexArray(0);
    }
};

#endif