#include "Menu.h"
#include <common/texture.h> // Use your texture loader (loadSOIL or similar)
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Menu::Menu() {}

Menu::~Menu() {
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &uvVBO);
    glDeleteVertexArrays(1, &VAO);
}

void Menu::init() {
    // 1. Setup a generic Quad (Square)
    GLfloat vertices[] = { -0.5, -0.5, 0,  0.5, -0.5, 0,  -0.5, 0.5, 0,
                           -0.5, 0.5, 0,   0.5, -0.5, 0,   0.5, 0.5, 0 };
    GLfloat uvs[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
    };

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glGenBuffers(1, &uvVBO);
    glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindVertexArray(0);

    // 2. Create Buttons
    // Make sure you have these images in your folder!
    Button startBtn;
    startBtn.position = glm::vec2(512, 400); // Middle of 1024x720 screen
    startBtn.size = glm::vec2(200, 80);
    startBtn.textureID = loadSOIL("photos/start.png"); 
    buttons.push_back(startBtn);
    
    Button exitBtn;
    exitBtn.position = glm::vec2(512, 250);
    exitBtn.size = glm::vec2(200, 80);
    exitBtn.textureID = loadSOIL("photos/exit.png");
    buttons.push_back(exitBtn);
    
}

void Menu::draw(GLuint shaderProgram, int windowWidth, int windowHeight) {
    glUseProgram(shaderProgram);
    
    // Enable Texture Mode
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);
    
    // Setup 2D Projection (0 to Width, 0 to Height)
    glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

    glBindVertexArray(VAO);

    for (const auto& btn : buttons) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(btn.position, 0.0f));
        model = glm::scale(model, glm::vec3(btn.size, 1.0f));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, btn.textureID);
        glUniform1i(glGetUniformLocation(shaderProgram, "myTextureSampler"), 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindVertexArray(0);
}

int Menu::checkClick(double mouseX, double mouseY, int windowHeight) {
    // Convert Mouse Y (Top-Left 0) to OpenGL Y (Bottom-Left 0)
    double glY = windowHeight - mouseY;

    for (int i = 0; i < buttons.size(); i++) {
        Button& b = buttons[i];
        // Simple AABB Collision
        if (mouseX >= b.position.x - b.size.x/2 && mouseX <= b.position.x + b.size.x/2 &&
            glY >= b.position.y - b.size.y/2 && glY <= b.position.y + b.size.y/2) {
            return i + 1; // 1 for Start, 2 for Exit
        }
    }
    return 0; // Missed
}