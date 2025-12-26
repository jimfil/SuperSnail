#include "Snail.h"
#include <glm/gtc/matrix_transform.hpp>
#include <common/model.h>
#include <iostream>

using namespace glm;

Snail::Snail(
    vec3 pos, float scalar, float mass){
    mesh = new Drawable("models/Mesh_Snail.obj");
    mesh_retracted = new Drawable("models/Mesh_Snail_Retracted.obj");
    isSprinting = false;
    isRetracted = false;
    s = scalar;
	radius = 1.73f * s; 
    m = mass;
    x = pos;
    P = m * v;
	moveSpeed = 5.0f;
	maxSpeed = 25.0f;
    staminaMax = 100.0f;
    stamina = 50.0f;
    staminaDepletionRate = 20.0f; 
    staminaRepletionRate = 15.0f;

    I_inv = mat3(1.0f / (0.4f * m * radius * radius));
}

Snail::~Snail() {
    delete mesh;
}

void Snail::draw() {
    if (isRetracted) {
        mesh_retracted->bind();
        mesh_retracted->draw();
    }
	else{
        mesh->bind();
        mesh->draw();
    }
    
    
}

void Snail::update(float t, float dt) {
    //integration
    advanceState(t, dt);
    if (length(this->w) < 0.05f) {
        this->w = vec3(0, 0, 0);
        this->L = vec3(0, 0, 0);
    }
    if (this->isSprinting && this->isMoving) {
        float staminaChange = -staminaDepletionRate * dt;
        stamina = glm::clamp(stamina + staminaChange, 0.0f, staminaMax);
        //std::cout << "DEPLETING: " << stamina << std::endl;
    }
    else {
        float staminaChange = staminaRepletionRate * dt;
        stamina = glm::clamp(stamina + staminaChange, 0.0f, staminaMax);
        //std::cout << "REFILLING: " << stamina << std::endl;
	}
    // compute model matrix
    mat4 scale = glm::scale(mat4(), vec3(s, s, s));
    mat4 tranlation = translate(mat4(), vec3(x.x, x.y, x.z));
#ifdef USE_QUATERNIONS
    mat4 rotation = mat4_cast(q);
#else
    mat4 rotation = mat4(R);
#endif
    snailModelMatrix = tranlation * rotation  * scale;
}


