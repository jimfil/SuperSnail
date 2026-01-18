#ifndef EAGLE_H
#define EAGLE_H

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <string>
#include <common/model.h> 

class Snail;

enum EagleState {
    PATROLLING,
    DIVING,
    GRABBING,
    RETURNING
};

class Eagle {
public:
    glm::vec3 position;
    glm::vec3 velocity;
    float speed;
    float rotationY;

    Drawable* model;
    EagleState state;
    float patrolTimer;
    float attackCooldown;    
    float diveSpeed;          
    glm::vec3 startDivePos;   
    bool hasSnail;
    Eagle(glm::vec3 startPos);
    ~Eagle();

    void update(float dt, Snail* snail);
    void draw(GLuint shaderID, GLuint modelLocation, GLuint colorLocation);

private:
    void updatePatrol(float dt);
};

#endif