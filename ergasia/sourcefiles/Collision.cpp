#include "Collision.h"
#include "Box.h"
#include "Sphere.h"
#include "heightmap.h"
#include "snail.h"
using namespace glm;

const float g = 9.80665f;

bool handleSnailTerrainCollision(Snail* snail, Heightmap* terrain,float dt) {
    if (!snail || !terrain) return false; // Safety check

    float groundHeight = terrain->getHeightAt(snail->x.x, snail->x.z);
    float radius = 1.73f;
    float snailBottom = snail->x.y - radius;
    vec3 targetUp;
    vec3 targetForward;

    if (!(snail->isRetracted)) {

        snail->x.y = groundHeight + radius + 0.01f;

        snail->v.y = 0;
        snail->v.x = 0;
        snail->v.z = 0;

        snail->P = snail->v * snail->m;

        
        targetUp = terrain->getNormalAt(snail->x.x, snail->x.z);

        targetForward = snail->q * vec3(0, 0, -1);

        vec3 newY = targetUp;
        vec3 newX = normalize(cross(targetForward, newY));
        if (length(newX) < 0.001f) newX = normalize(cross(vec3(1, 0, 0), newY));
        vec3 newZ = normalize(cross(newX, newY));

        mat3 targetRotMat;
        targetRotMat[0] = newX;
        targetRotMat[1] = newY;
        targetRotMat[2] = newZ;

        quat targetQ = quat_cast(targetRotMat);
        snail->q = normalize(slerp(snail->q, targetQ, 0.2f)); // Increased smoothing speed slightly

        
        
        return true; 
    }

    //Since we didnt return, we are retracted!
    

    if (snailBottom < groundHeight) {

		targetUp = terrain->getNormalAt(snail->x.x, snail->x.z);

        snail->x.y = groundHeight + radius + 0.01f;
        

        return true; 
    }


    return false; 

}