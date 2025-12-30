#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

struct Button {
    glm::vec2 position; // Screen position (pixels)
    glm::vec2 size;     // Width/Height (pixels)
    GLuint textureID;
};

class Menu {
public:
    Menu();
    ~Menu();

    // Load textures for the buttons
    void init();
    
    // Draw all buttons
    void draw(GLuint shaderProgram, int windowWidth, int windowHeight);
    
    // Returns: 0=Nothing, 1=Start, 2=Exit
    int checkClick(double mouseX, double mouseY, int windowHeight);

private:
    GLuint VAO, VBO, uvVBO;
    std::vector<Button> buttons;
};