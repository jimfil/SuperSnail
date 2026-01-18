#include "Eagle.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "Snail.h"

using namespace glm;

Eagle::Eagle(vec3 startPos) {
    position = startPos;
    velocity = vec3(1.0f, 0.0f, 0.0f);
    speed = 15.0f;
    diveSpeed = 40.0f;
    state = PATROLLING;
    patrolTimer = 0.0f;
    rotationY = 0.0f;
    attackCooldown = 0.0f;
    hasSnail = false;

    model = new Drawable("models/eagle.obj");
}

Eagle::~Eagle() {
    delete model;
}

void Eagle::update(float dt, Snail* snail) {
    float distToSnail = distance(vec3(position.x, 0, position.z), vec3(snail->x.x, 0, snail->x.z));
    vec3 directionToSnail = normalize(snail->x - position);

    switch (state) {
    case PATROLLING:
        updatePatrol(dt);
        position += velocity * dt;
        if (distToSnail < 100.0f && attackCooldown <= 0.0f) {
            state = DIVING;
            startDivePos = position;
            velocity = directionToSnail * diveSpeed;
        }

        if (attackCooldown > 0) attackCooldown -= dt;
        break;

    case DIVING:
        velocity = directionToSnail * diveSpeed;
        position += velocity * dt;

        if (distance(position, snail->x) < 3.0f && snail->isRetracted) {
            state = GRABBING;
            hasSnail = true;
        }

        if (position.y < snail->x.y + 1.0f) {
            state = RETURNING;
        }
        break;

    case GRABBING:
        velocity = vec3(1.0f, 20.0f, -1.0f);
        position += velocity * dt;

        snail->x = position - vec3(0, 2.0f, 0);
        snail->v = vec3(0);

        if (position.y > 150.0f || !snail->isRetracted) {
            hasSnail = false;
            state = PATROLLING;
            attackCooldown = 10.0f;
            snail->v = vec3(-3.0f, -5.0f, 3.0f);
        }
        break;

    case RETURNING:
        if (position.y < 130.0f) {
            velocity = vec3(0.0f, 15.0f, 0.0f);
            position += velocity * dt;
        }
        else {
            state = PATROLLING;
            attackCooldown = 5.0f;
        }
        break;
    }

    if (length(velocity) > 0.1f) {
        rotationY = -atan2(velocity.x, velocity.z);
    }
}

void Eagle::updatePatrol(float dt) {
    patrolTimer += dt;

    float radius = 50.0f;
    float centerX = 0.0f;
    float centerZ = 0.0f;

    float targetX = centerX + sin(patrolTimer * 0.5f) * radius;
    float targetZ = centerZ + cos(patrolTimer * 0.5f) * radius;

    float targetY = 130.0f;

    vec3 targetPos(targetX, targetY, targetZ);
    vec3 dir = normalize(targetPos - position);

    velocity = dir * speed;
}

void Eagle::draw(GLuint shaderProgram, GLuint modelLocation, GLuint colorLocation) {
    
    model->bind();
    mat4 modelMat = mat4(1.0f);
    modelMat = translate(modelMat, position);
    modelMat = rotate(modelMat, rotationY, vec3(0, 1, 0));
    modelMat = scale(modelMat, vec3(0.1f));

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &modelMat[0][0]);

    glUniform4f(colorLocation, 0.7f, 0.0f, 0.0f, 1.0f);

    model->draw();
}