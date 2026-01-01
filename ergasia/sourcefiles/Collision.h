#ifndef COLLISION_H
#define COLLISION_H

#include <glm/glm.hpp>
#include "heightmap.h"
class Snail;

void handleBoxSnailCollision(Heightmap* heightmap, Snail* snail);
bool checkForBoxSnailCollision(glm::vec3& pos, const float& r, const float& size, glm::vec3& n);
bool handleSnailTerrainCollision(Snail* snail, Heightmap* terrain, bool onTree);

bool handleSnailTreeCollision(Snail* snail, const std::vector<glm::mat4>& treeMatrices);

#endif
