#ifndef SNAIL_H
#define SNAIL_H

#include "RigidBody.h"

class Drawable;

class Snail : public RigidBody {
public:
    Drawable *mesh;
    float b, k, s;
    glm::vec3 a;
    glm::mat4 snailModelMatrix;

    Snail(glm::vec3 pos, glm::vec3 vel, float scalar, float mass,
        glm::vec3 anchor, float stiffness, float damping);
    ~Snail();

    void draw();
    void update(float t = 0, float dt = 0);

};

#endif