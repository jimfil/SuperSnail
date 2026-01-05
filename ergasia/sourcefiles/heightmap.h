#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "common/model.h" 

class Heightmap : public Drawable {
public:
    struct HillAlgorithmParameters {
        HillAlgorithmParameters(int rows, int columns, int numHills, int rMin, int rMax, float hMin, float hMax, float s, float sY)
            : rows(rows), columns(columns), numHills(numHills), hillRadiusMin(rMin), hillRadiusMax(rMax), hillMinHeight(hMin), hillMaxHeight(hMax), scalar(s), scalarY(sY){
        }
        int rows, columns, numHills, hillRadiusMin, hillRadiusMax;
        float hillMinHeight, hillMaxHeight,scalar, scalarY;
    };
    float scalar, scalarY;
    glm::vec3 position; 

    std::vector<std::vector<float>> heightGrid;
    std::vector<std::vector<float>> typeGrid;

    GLuint splatTextureID;

    int rows, cols;

    // Public Constructor
    Heightmap(const HillAlgorithmParameters& params);
    ~Heightmap();

    glm::mat4 returnplaneMatrix();
    float getHeightAt(float worldX, float worldZ);
    glm::vec3 getNormalAt(float worldX, float worldZ);
    float getGroundTypeAt(float worldX, float worldZ);
private:
    // Helper struct to hold data temporarily
    struct MeshData {
        std::vector<glm::vec3> v;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> n;

        std::vector<std::vector<float>> grid;
        std::vector<std::vector<float>> typeGrid;
    };

    // Private Constructor (The target of delegation)
    Heightmap(const MeshData& data);

    // Static function to do the calculation BEFORE construction
    static MeshData generate(const HillAlgorithmParameters& params);
};