#include "Collision.h"
#include "Box.h"
#include "Sphere.h"
#include "heightmap.h"
#include "snail.h"
using namespace glm;

bool handleSnailTerrainCollision(Snail* snail, Heightmap* terrain,bool stopMovement) {
    if (!snail || !terrain) return false; // Safety check

    // 1. Ask terrain for height
    float groundHeight = terrain->getHeightAt(snail->x.x, snail->x.z);

    // If falling off map, return false
    if (groundHeight < -5000.0f) return false;

    // 2. Geometry
    float radius = snail->s;
    float snailBottom = snail->x.y - radius;

    // 3. Collision Response
    if (snailBottom < groundHeight) {

        // --- FIX 1: Position Stability ---
        // Snap to top + epsilon
        snail->x.y = groundHeight + radius + 0.001f;

        snail->v.y = 0;
        snail->v.x = 0;
        snail->v.z = 0;

        // Sync Physics State
        snail->P = snail->v * snail->m;

        // ---------------------------------------------------------
        // 4. ROTATION ALIGNMENT
        // ---------------------------------------------------------
        // (Your existing rotation code is good, keep it here)
        glm::vec3 targetUp = terrain->getNormalAt(snail->x.x, snail->x.z);
        glm::vec3 targetForward;

        // Since velocity is 0, always use current rotation for forward
        targetForward = snail->q * glm::vec3(0, 0, -1); // Assuming -1 is model forward

        glm::vec3 newY = targetUp;
        glm::vec3 newX = glm::normalize(glm::cross(targetForward, newY));
        if (glm::length(newX) < 0.001f) newX = glm::normalize(glm::cross(glm::vec3(1, 0, 0), newY));
        glm::vec3 newZ = glm::normalize(glm::cross(newX, newY));

        glm::mat3 targetRotMat;
        targetRotMat[0] = newX;
        targetRotMat[1] = newY;
        targetRotMat[2] = newZ;

        glm::quat targetQ = glm::quat_cast(targetRotMat);
        snail->q = glm::normalize(glm::slerp(snail->q, targetQ, 0.2f)); // Increased smoothing speed slightly

        if (stopMovement) {
            // Kill ALL velocity if no keys are pressed (Snail Sticks)
            snail->v = vec3(0.0f);
        }
        else {
            // If moving, only kill downward velocity (The more complex tangential fix)
            vec3 surfaceNormal = terrain->getNormalAt(snail->x.x, snail->x.z);
            float normalVelocity = dot(snail->v, surfaceNormal);
            if (normalVelocity < 0) {
                snail->v -= surfaceNormal * normalVelocity;
            }
        }

        return true; // <--- WE ARE ON THE GROUND
    }

    return false; // <--- WE ARE IN THE AIR
}