#ifndef COLLISION_H
#define COLLISION_H

#include <glm/glm.hpp>
#include "Snail.h"
#include "heightmap.h"
class Box;
class Sphere;
bool handleSnailTerrainCollision(Snail* snail, Heightmap* terrain,bool stopMovement);

#endif
