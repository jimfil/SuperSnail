#ifndef SNAIL_H
#define SNAIL_H

#include "RigidBody.h"

class Drawable;

class Snail : public RigidBody {
public:
    Drawable *mesh,*mesh_retracted;
    float s;
    glm::mat4 snailModelMatrix;
	bool isRetracted, isSprinting, isMoving;
    float radius;
	float moveSpeed, maxSpeed;
	float stamina, staminaMax , staminaDepletionRate, staminaRepletionRate;
    Snail(glm::vec3 pos, float scalar, float mass);
    ~Snail();
    void draw();
    void update(float t = 0, float dt = 0);
};

#endif