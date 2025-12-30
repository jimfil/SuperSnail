#ifndef SNAIL_H
#define SNAIL_H

#include "RigidBody.h"

class Drawable;

class Snail : public RigidBody {
public:
    Drawable *mesh,*mesh_retracted;
    float s;
    glm::mat4 snailModelMatrix;
	bool isRetracted, isSprinting, isMoving, abilityUnlocked;
    float retractTarget , retractCurrent; // Target for retracting (1.0 = fully retracted), Current animation progress (0.0 to 1.0)
    const float retractSpeed = 2.0f ; // Speed of retract/extend animation per second
    float radius;
	float moveSpeed, maxSpeed;
	float stamina, staminaMax , staminaDepletionRate, staminaRepletionRate;
    Snail(glm::vec3 pos, float scalar, float mass);
    ~Snail();
    void draw();
    void update(float t = 0, float dt = 0);
};

#endif