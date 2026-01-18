#include "Menu.h"
#include <common/texture.h> 
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>

using namespace glm;

Menu::Menu() {
    pages.resize(2); 
}

void Menu::addButton(int pageID, vec2 pos, vec2 size, GLuint textureID, int actionID) {
    Button b;
    b.position = pos;
    b.size = size;
    b.textureID = textureID;
    b.actionID = actionID; 
    pages[pageID].push_back(b);
}
Menu::~Menu() {
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &uvVBO);
    glDeleteVertexArrays(1, &VAO);
}

void Menu::init(int w,int h) {
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
    addButton(0, vec2(w * 0.5f, h * 0.55f), vec2(150, 150), loadSOIL("textures/start.png"), 1);

    addButton(0, vec2(w * 0.5f, h * 0.87f), vec2(200, 80), loadSOIL("textures/start2.png"), 3);

    addButton(0, vec2(w * 0.5f, h * 0.35f), vec2(200, 80), loadSOIL("textures/exit.png"), 2);


    //SETTINGS MENU 
    addButton(1, vec2(w * 0.5f, h * 0.21f), vec2(150, 150), loadSOIL("textures/start.png"), 1);

    // Tree Controls 
    addButton(1, vec2(w * 0.59f, h * 0.70f), vec2(50, 50), loadSOIL("textures/plus.png"), 4);

    addButton(1, vec2(w * 0.39f, h * 0.70f), vec2(50, 50), loadSOIL("textures/minus.png"), 5);
    
    addButton(1, vec2(w * 0.59f, h * 0.40f), vec2(50, 50), loadSOIL("textures/plus.png"), 6);
    
    addButton(1, vec2(w * 0.39f, h * 0.40f), vec2(50, 50), loadSOIL("textures/minus.png"), 7);

}


void Menu::drawIcon(GLuint shaderProgram, GLuint textureID, glm::vec2 pos, glm::vec2 size) {
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos, 0.0f));
    model = glm::scale(model, glm::vec3(size, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(shaderProgram, "myTextureSampler"), 0);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Menu::draw(GLuint shaderProgram, int windowWidth, int windowHeight, int pageID) {
    glUseProgram(shaderProgram);
    
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);
    
    glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

    glBindVertexArray(VAO);

    for (const auto& btn : pages[pageID] ) {
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

int Menu::checkClick(double mouseX, double mouseY, int windowHeight, int pageID) {
    double glY = windowHeight - mouseY;
    for (const auto& b : pages[pageID]) {
        if (mouseX >= b.position.x - b.size.x / 2 && mouseX <= b.position.x + b.size.x / 2 &&
            glY >= b.position.y - b.size.y / 2 && glY <= b.position.y + b.size.y / 2) {
            return b.actionID;
        }
    }
    return 0; // Missed
}


void Menu::initText(const char* texturePath) {
    numberTextureID = loadSOIL(texturePath);

    GLfloat vertices[] = { 0,0,0,
                           1,0,0,
                           0,1,0,
                           0,1,0,
                           1,0,0,
                           1,1,0 };

    GLfloat uvs[] = { 0,1, 1,1, 0,0,  0,0, 1,1, 1,0 };

    glGenVertexArrays(1, &numberVAO);
    glBindVertexArray(numberVAO);

    glGenBuffers(1, &numberVBO);
    glBindBuffer(GL_ARRAY_BUFFER, numberVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glGenBuffers(1, &numberUVVBO);
    glBindBuffer(GL_ARRAY_BUFFER, numberUVVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindVertexArray(0);
}

void Menu::drawNumber(GLuint shaderProgram, int number, glm::vec2 pos, float scale,int w, int h) {
	glUseProgram(shaderProgram);
    std::string s = std::to_string(number);
    float spacing = 30.0f * scale; // Distance between digits
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, numberTextureID);
    glUniform1i(glGetUniformLocation(shaderProgram, "myTextureSampler"), 0);

    glBindVertexArray(numberVAO);
    mat4 projection = glm::ortho(0.0f, (float)w, 0.0f, (float)h);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

    for (int i = 0; i < s.length(); i++) {
        int digit = s[i] - '0';

        float left = digit / 10.0f;      
        float right = (digit + 1) / 10.0f;

        GLfloat uvs[] = {
            left, 1.0f,  right, 1.0f,  left, 0.0f,
            left, 0.0f,  right, 1.0f,  right, 0.0f
        };

        glBindBuffer(GL_ARRAY_BUFFER, numberUVVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(uvs), uvs);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(pos.x + (i * spacing), pos.y, 0.0f));
        model = glm::scale(model, glm::vec3(30.0f * scale, 50.0f * scale, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindVertexArray(0);
}