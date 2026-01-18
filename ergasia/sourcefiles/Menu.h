#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <common/model.h>

struct Button {
    glm::vec2 position; 
    glm::vec2 size;    
    GLuint textureID;
    int actionID;
};

struct Icon {
    glm::vec2 position;
    glm::vec2 size;
    GLuint textureID;
};

class Menu {
public:
    Menu();
    ~Menu();

    void init(int width, int height);
    
    void draw(GLuint shaderProgram, int windowWidth, int windowHeight, int pageID);
    int checkClick(double mouseX, double mouseY, int windowHeight, int pageID);

    void addButton(int pageID, glm::vec2 pos, glm::vec2 size, GLuint textureID, int actionID);
    void initText(const char* texturePath);
    void drawNumber(GLuint shaderID, int number, glm::vec2 pos, float scale,int w, int h);
    void drawIcon(GLuint shaderProgram, GLuint textureID, glm::vec2 pos, glm::vec2 size);
private:
    GLuint VAO, VBO, uvVBO;
    std::vector<std::vector<Button>> pages;

    GLuint numberTextureID;
    GLuint numberVAO, numberVBO, numberUVVBO;

};