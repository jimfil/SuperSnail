#include "Collision.h"
#include "Box.h"
#include "Sphere.h"
#include "heightmap.h"
#include "snail.h"
using namespace glm;

bool handleSnailTerrainCollision(Snail* snail, Heightmap* terrain) {
    if (!snail || !terrain) return false; // Safety check

    float groundHeight = terrain->getHeightAt(snail->x.x, snail->x.z);
    float radius = 1.53f;
    float snailBottom = snail->x.y - radius;

    if (!(snail->isRetracted)) {

        snail->x.y = groundHeight + radius + 0.15f;

        snail->v.y = 0;
        snail->v.x = 0;
        snail->v.z = 0;

        snail->P = snail->v * snail->m;

        glm::vec3 targetUp = terrain->getNormalAt(snail->x.x, snail->x.z);
        glm::vec3 targetForward;

        targetForward = snail->q * glm::vec3(0, 0, -1);

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

        
        
        return true; 
    }

    //Since we didnt return, we are retracted!
    

    if (snailBottom < groundHeight) {


        snail->x.y = groundHeight + radius;

        snail->v.y *= 0.8;
        snail->v.x *= 0.8;
        snail->v.z *= 0.8;

        snail->P = snail->v * snail->m;

        glm::vec3 targetUp = terrain->getNormalAt(snail->x.x, snail->x.z);
        glm::vec3 targetForward;

        targetForward = snail->q * glm::vec3(0, 0, -1); 

        glm::vec3 newY = targetUp;
        glm::vec3 newX = glm::normalize(glm::cross(targetForward, newY));
        if (glm::length(newX) < 0.001f) newX = glm::normalize(glm::cross(glm::vec3(1, 0, 0), newY));
        glm::vec3 newZ = glm::normalize(glm::cross(newX, newY));

        glm::mat3 targetRotMat;
        targetRotMat[0] = newX;
        targetRotMat[1] = newY;
        targetRotMat[2] = newZ;

        glm::quat targetQ = glm::quat_cast(targetRotMat);
        snail->q = glm::normalize(glm::slerp(snail->q, targetQ, 0.2f));

   
        

        return true; 
    }


    return false; 

}