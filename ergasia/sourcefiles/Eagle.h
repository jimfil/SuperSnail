#ifndef EAGLE_H
#define EAGLE_H

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <string>
#include <common/model.h> // Make sure this points to where your Drawable class is defined

// Forward declaration to avoid circular dependency
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

    // REPLACED: Manual buffers with your class
    Drawable* model;
    // AI State
    EagleState state;
    float patrolTimer;
    float attackCooldown;     // Time before eagle can attack again
    float diveSpeed;          // Faster speed when diving
    glm::vec3 startDivePos;   // Where the dive started (to return to)
    bool hasSnail;
    Eagle(glm::vec3 startPos);
    ~Eagle();

    void update(float dt, Snail* snail);
    void draw(GLuint shaderID, GLuint modelLocation, GLuint colorLocation);

private:
    void updatePatrol(float dt);
};

#endif