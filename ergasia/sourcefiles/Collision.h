#ifndef COLLISION_H
#define COLLISION_H

#include <glm/glm.hpp>
#include "heightmap.h"
#include <unordered_map>
#include <vector>

class Snail;

struct GridKey {
    int x, z;
    bool operator==(const GridKey& other) const {
        return x == other.x && z == other.z;
    }
};

struct GridKeyHash {
    std::size_t operator()(const GridKey& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.z) << 1);
    }
};

extern std::unordered_map<GridKey, std::vector<int>, GridKeyHash> treeGrid;
extern float cellSize; 

void handleBoxSnailCollision(Heightmap* heightmap, Snail* snail);
bool checkForBoxSnailCollision(glm::vec3& pos, const float& r, const float& size, glm::vec3& n);
bool handleSnailTerrainCollision(Snail* snail, Heightmap* terrain, bool onTree);

bool handleSnailTreeCollision(Snail* snail, const std::vector<glm::mat4>& treeMatrices);

#endif
