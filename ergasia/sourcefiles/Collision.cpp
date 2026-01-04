#include "Collision.h"
#include "heightmap.h"
#include "snail.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace glm;

const float g = 9.80665f;


bool checkForBoxSnailCollision(vec3& pos, const float& r, const float& size, vec3& n) {
    bool collided = false;
    vec3 totalNormal(0.0f); 

    if (pos.x - r < -size) {
        pos.x = -size + r;
        totalNormal += vec3(1, 0, 0); 
        collided = true;
    }
    else if (pos.x + r > size) {
        pos.x = size - r;
        totalNormal += vec3(-1, 0, 0);
        collided = true;
    }

    if (pos.z - r < -size) {
        pos.z = -size + r;
        totalNormal += vec3(0, 0, 1);
        collided = true;
    }
    else if (pos.z + r > size) {
        pos.z = size - r;
        totalNormal += vec3(0, 0, -1);
        collided = true;
    }

    if (collided) {
        n = normalize(totalNormal);
        return true;
    }

    return false;
}


void handleBoxSnailCollision(Heightmap* heightmap, Snail* snail) {
    vec3 n;
    if (checkForBoxSnailCollision(snail->x, snail->radius, heightmap->scalar/2, n)) {
        snail->v = snail->v - 2.0f * dot(snail->v, n) * n;
        snail->P = snail->v * snail->m;
    }
}


bool handleSnailTerrainCollision(Snail* snail, Heightmap* terrain, bool onTree) {
    if (!snail || !terrain) return false; // Safety check

    float groundHeight = terrain->getHeightAt(snail->x.x, snail->x.z);
    float radius = snail->radius;
    float snailBottom = snail->x.y - radius;
    vec3 targetUp;
    vec3 targetForward;
    
    if (!(snail->isRetracted)) {
        
        snail->v.y = 0;
        snail->v.x = 0;
        snail->v.z = 0;

        snail->P = snail->v * snail->m;
        if (onTree && snailBottom > groundHeight) {
            return true;
        }


        targetUp = terrain->getNormalAt(snail->x.x, snail->x.z);
        snail->x.y = (groundHeight + radius + 0.04f);
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
        if (length(snail->v) > 40.0f) {
            snail->x.y = groundHeight + radius + 0.2f;
        }
        snail->x.y = groundHeight + radius + 0.03f;
        return true; 
    }

    return false; 

}

bool handleSnailTreeCollision(Snail* snail, const std::vector<mat4>& instanceMatrices) {
    float treeRadius = 0.3f;
    float combinedRadius = snail->radius + treeRadius + 1.0f;
    float detectionDist = combinedRadius + 0.1f;

    float treeHeight = 20.0f;


    for (const auto& modelMatrix : instanceMatrices) {
        vec3 treePos = vec3(modelMatrix[3]);
        float treeTopY = treePos.y + treeHeight; // Absolute world Y of the top

        // Calculate distance on XZ Plane
        float dx = snail->x.x - treePos.x;
        float dz = snail->x.z - treePos.z;
        float distSq = dx * dx + dz * dz;

        // ZONE 1: TOP CAP (Walkable Platform)
        if (abs(snail->x.y - (treeTopY + snail->radius)) < 0.5f && distSq < detectionDist * detectionDist) {
             snail->x.y = treeTopY - 0.1 ;
            vec3 surfaceNormal = vec3(0, 1, 0);

            return true; // We found a collision, stop checking others
        }


        // ZONE 2: SIDE TRUNK (Climbable Wall)
        if (snail->x.y < (treeTopY + snail->radius) && distSq < detectionDist * detectionDist) {
            

            float distance = sqrt(distSq);
            float normalX = dx / distance;
            float normalZ = dz / distance;
            snail->x.x = treePos.x + (normalX * combinedRadius);
            snail->x.z = treePos.z + (normalZ * combinedRadius);
            vec3 treeNormal = vec3(normalX, 0.0f, normalZ);
            //clip off from the tree
            float heightFromBase = snail->x.y - treePos.y;
            vec3 forward = snail->q * vec3(0, 0, 1);
            if (!snail->isRetracted && forward.y < -0.2f && heightFromBase < 2.5f) {
                return false; 
            }


            if (snail->isRetracted) {
                if (dot(snail->v, treeNormal) < 0.0f) {
                    snail->v = reflect(snail->v, treeNormal) * 1.0f;
                    snail->P = snail->v * snail->m;
                }
            }
            else {
                
                //if (dot(snail->v, treeNormal) > 0.1f) return false;
                // Rotate Snail Sideways
                vec3 targetUp = treeNormal;
                vec3 currentForward = snail->q * vec3(0, 0, -1);

                vec3 newY = targetUp;
                vec3 newX = normalize(cross(currentForward, newY));
                if (length(newX) < 0.001f) newX = normalize(cross(vec3(0, 1, 0), newY));
                vec3 newZ = normalize(cross(newX, newY));

                mat3 targetRotMat;
                targetRotMat[0] = newX;
                targetRotMat[1] = newY;
                targetRotMat[2] = newZ;

                quat targetQ = quat_cast(targetRotMat);
                snail->q = normalize(slerp(snail->q, targetQ, 0.2f));
            }
            return true;
        }
    }
    return false;
}