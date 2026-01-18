#include "Menu.h"
#include <common/texture.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>

using namespace glm;
using namespace std;

Menu::Menu() {
    pages.resize(2);
    uiQuad = nullptr;
    for (int i = 0; i < 10; i++) digitQuads[i] = nullptr;
}

Menu::~Menu() {
    if (uiQuad) delete uiQuad;
    for (int i = 0; i < 10; i++) {
        if (digitQuads[i]) delete digitQuads[i];
    }
}

void Menu::addButton(int pageID, vec2 pos, vec2 size, GLuint textureID, int actionID) {
    Button b;
    b.position = pos;
    b.size = size;
    b.textureID = textureID;
    b.actionID = actionID;
    pages[pageID].push_back(b);
}

void Menu::init(int w, int h) {
    vector<vec3> vertices = {
        vec3(-0.5, -0.5, 0), vec3(0.5, -0.5, 0), vec3(-0.5, 0.5, 0),
        vec3(-0.5, 0.5, 0),  vec3(0.5, -0.5, 0), vec3(0.5, 0.5, 0)
    };

    vector<vec2> uvs = {
        vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(0.0f, 0.0f),
        vec2(0.0f, 0.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f)
    };

    vector<vec3> normals(6, vec3(0, 0, 1));

    uiQuad = new Drawable(vertices, uvs, normals);

    addButton(0, vec2(w * 0.5f, h * 0.55f), vec2(150, 150), loadSOIL("textures/start.png"), 1);
    addButton(0, vec2(w * 0.5f, h * 0.87f), vec2(200, 80), loadSOIL("textures/start2.png"), 3);
    addButton(0, vec2(w * 0.5f, h * 0.35f), vec2(200, 80), loadSOIL("textures/exit.png"), 2);

    addButton(1, vec2(w * 0.5f, h * 0.21f), vec2(150, 150), loadSOIL("textures/start.png"), 1);
    addButton(1, vec2(w * 0.59f, h * 0.70f), vec2(50, 50), loadSOIL("textures/plus.png"), 4);
    addButton(1, vec2(w * 0.39f, h * 0.70f), vec2(50, 50), loadSOIL("textures/minus.png"), 5);
    addButton(1, vec2(w * 0.59f, h * 0.40f), vec2(50, 50), loadSOIL("textures/plus.png"), 6);
    addButton(1, vec2(w * 0.39f, h * 0.40f), vec2(50, 50), loadSOIL("textures/minus.png"), 7);
}

void Menu::initText(const char* texturePath) {
    numberTextureID = loadSOIL(texturePath);

    vector<vec3> vertices = {
        vec3(0,0,0), vec3(1,0,0), vec3(0,1,0),
        vec3(0,1,0), vec3(1,0,0), vec3(1,1,0)
    };
    vector<vec3> normals(6, vec3(0, 0, 1));

    for (int i = 0; i < 10; i++) {
        float left = i / 10.0f;
        float right = (i + 1) / 10.0f;

        vector<vec2> digitUVs = {
            vec2(left, 1.0f), vec2(right, 1.0f), vec2(left, 0.0f),
            vec2(left, 0.0f), vec2(right, 1.0f), vec2(right, 0.0f)
        };

        digitQuads[i] = new Drawable(vertices, digitUVs, normals);
    }
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

    uiQuad->bind();
    uiQuad->draw();
}

void Menu::draw(GLuint shaderProgram, int windowWidth, int windowHeight, int pageID) {
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);

    glm::mat4 projection = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

    uiQuad->bind();

    for (const auto& btn : pages[pageID]) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(btn.position, 0.0f));
        model = glm::scale(model, glm::vec3(btn.size, 1.0f));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, btn.textureID);
        glUniform1i(glGetUniformLocation(shaderProgram, "myTextureSampler"), 0);

        uiQuad->draw();
    }
}

void Menu::drawNumber(GLuint shaderProgram, int number, glm::vec2 pos, float scale, int w, int h) {
    glUseProgram(shaderProgram);
    std::string s = std::to_string(number);
    float spacing = 30.0f * scale;

    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, numberTextureID);
    glUniform1i(glGetUniformLocation(shaderProgram, "myTextureSampler"), 0);

    mat4 projection = glm::ortho(0.0f, (float)w, 0.0f, (float)h);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

    for (int i = 0; i < s.length(); i++) {
        int digit = s[i] - '0';
        if (digit < 0 || digit > 9) continue;

        Drawable* currentDigit = digitQuads[digit];
        currentDigit->bind();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(pos.x + (i * spacing), pos.y, 0.0f));
        model = glm::scale(model, glm::vec3(30.0f * scale, 50.0f * scale, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);

        currentDigit->draw();
    }
}

int Menu::checkClick(double mouseX, double mouseY, int windowHeight, int pageID) {
    double glY = windowHeight - mouseY;
    for (const auto& b : pages[pageID]) {
        if (mouseX >= b.position.x - b.size.x / 2 && mouseX <= b.position.x + b.size.x / 2 &&
            glY >= b.position.y - b.size.y / 2 && glY <= b.position.y + b.size.y / 2) {
            return b.actionID;
        }
    }
    return 0;
}