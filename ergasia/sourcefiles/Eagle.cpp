#include "Eagle.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "Snail.h" // Include full definition for interaction

using namespace glm;

Eagle::Eagle(vec3 startPos) {
    position = startPos;
    velocity = vec3(1.0f, 0.0f, 0.0f);
    speed = 15.0f;          // Normal Patrol Speed
    diveSpeed = 40.0f;      // Fast Attack Speed
    state = PATROLLING;
    patrolTimer = 0.0f;
    rotationY = 0.0f;
    attackCooldown = 0.0f;  // Ready to attack immediately
    hasSnail = false;

    model = new Drawable("models/eagle.obj");
}

Eagle::~Eagle() {
    // Cleanup is now handled by the Drawable destructor
    delete model;
}

void Eagle::update(float dt, Snail* snail) {
    float distToSnail = distance(vec3(position.x, 0, position.z), vec3(snail->x.x, 0, snail->x.z));
    vec3 directionToSnail = normalize(snail->x - position);

    switch (state) {
    case PATROLLING:
        updatePatrol(dt);

        // Trigger Dive if close enough and cooldown is ready
        if (distToSnail < 100.0f && attackCooldown <= 0.0f) {
            state = DIVING;
            startDivePos = position; // Remember where we came from
            velocity = directionToSnail * diveSpeed;
        }

        // Reduce cooldown
        if (attackCooldown > 0) attackCooldown -= dt;
        break;

    case DIVING:
        // Fly straight towards the snail
        velocity = directionToSnail * diveSpeed;
        position += velocity * dt;

        // HIT CHECK: If we are very close to the snail
        if (distance(position, snail->x) < 3.0f && snail->isRetracted) {
            state = GRABBING;
            hasSnail = true;
        }

        // MISS CHECK: If we hit the ground (y < terrain height + 1) without grabbing
        if (position.y < snail->x.y + 1.0f) {
            state = RETURNING; // Missed, fly back up
        }
        break;

    case GRABBING:
        // Fly Upwards Carrying Snail
        velocity = vec3(1.0f, 20.0f, -1.0f); // Straight Up
        position += velocity * dt;

        // Lock Snail Position to Eagle's claws
        snail->x = position - vec3(0, 2.0f, 0); // Hang slightly below
        snail->v = vec3(0); // Kill snail physics velocity

        // Drop snail after reaching height
        if (position.y > 200.0f || !snail->isRetracted) {
            hasSnail = false;
            state = PATROLLING;
            attackCooldown = 10.0f; // Wait 10 seconds before next attack

            // Give snail a small drop velocity so it falls naturally
            snail->v = vec3(-3.0f, -5.0f, 3.0f);
        }
        break;

    case RETURNING:
        // Fly back to patrol altitude (e.g., 40.0f)
        if (position.y < 130.0f) {
            velocity = vec3(0.0f, 15.0f, 0.0f); // Fly Up
            position += velocity * dt;
        }
        else {
            state = PATROLLING;
            attackCooldown = 5.0f; // Short cooldown after a miss
        }
        break;
    }

    // --- Visuals: Rotation ---
    // If moving, face direction of movement
    if (length(velocity) > 0.1f) {
        rotationY = atan2(velocity.x, velocity.z);
    }
}

void Eagle::updatePatrol(float dt) {
    patrolTimer += dt;

    // Simple Circular Flight Pattern
    float radius = 50.0f;
    float centerX = 0.0f;
    float centerZ = 0.0f;

    // Calculate desired position in the circle
    float targetX = centerX + sin(patrolTimer * 0.5f) * radius;
    float targetZ = centerZ + cos(patrolTimer * 0.5f) * radius;

    // Fly at a fixed height
    float targetY = 130.0f;

    vec3 targetPos(targetX, targetY, targetZ);
    vec3 dir = normalize(targetPos - position);

    // Smoothly turn towards target
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