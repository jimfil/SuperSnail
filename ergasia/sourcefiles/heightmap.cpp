#include "heightmap.h"
#include <random>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

using namespace std;
using namespace glm;

Heightmap::Heightmap(const HillAlgorithmParameters& params)
    : Heightmap(generate(params))
{
    this->scalar = params.scalar;
    this->scalarY = params.scalarY;
    this->rows = params.rows;
    this->cols = params.columns;
    this->position = glm::vec3(0.0f, 0.0f, 0.0f);

    // splat map
    glGenTextures(1, &splatTextureID);
    glBindTexture(GL_TEXTURE_2D, splatTextureID);


    std::vector<unsigned char> imageBuffer;
    imageBuffer.reserve(rows * cols * 3); // Reserve for RGB 

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            float val = this->typeGrid[r][c];

            unsigned char red = 0;
            unsigned char green = 0;
            unsigned char blue = 0;

            if (val > 0.5f) {       // Rock (1.0)
                red = 255;
            }
            else if (val < -0.5f) { // Bouncy (-1.0)
                green = 255;
            }
            // Else it stays black (Grass)

            imageBuffer.push_back(red);
            imageBuffer.push_back(green);
            imageBuffer.push_back(blue);
        }
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cols, rows, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageBuffer[0]);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

Heightmap::Heightmap(const MeshData& data)
    : Drawable(data.v, data.uv, data.n)
{
    rows = 0; cols = 0; scalar = 0; scalarY = 0;
    this->heightGrid = data.grid;
    this->typeGrid = data.typeGrid; // Copy the type grid
    this->splatTextureID = 0;
}

Heightmap::~Heightmap() {
    if (splatTextureID != 0) glDeleteTextures(1, &splatTextureID);
}

Heightmap::MeshData Heightmap::generate(const HillAlgorithmParameters& params)
{
    MeshData data;
    // Initialize grids
    std::vector<std::vector<float>> grid(params.rows, std::vector<float>(params.columns, 0.0f));
    std::vector<std::vector<float>> typeGrid(params.rows, std::vector<float>(params.columns, 0.0f));

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> rDist(params.hillRadiusMin, params.hillRadiusMax);
    std::uniform_real_distribution<float> hDist(params.hillMinHeight, params.hillMaxHeight);
    std::uniform_int_distribution<int> rowDist(0, params.rows - 1);
    std::uniform_int_distribution<int> colDist(0, params.columns - 1);
    //gen Hills
    for (int i = 0; i < params.numHills; i++) {
        int cR = rowDist(generator);
        int cC = colDist(generator);
        int rad = rDist(generator);
        float hillH = hDist(generator);

        for (int r = cR - rad; r < cR + rad; r++) {
            for (int c = cC - rad; c < cC + rad; c++) {
                if (r < 0 || r >= params.rows || c < 0 || c >= params.columns) continue;
                float r2 = float(rad * rad);
                float dx = float(cC - c);
                float dy = float(cR - r);
                float hVal = (r2 - dx * dx - dy * dy) / 5;
                if (hVal > 0.0f) {
                    grid[r][c] += hillH * (hVal / r2);
                    if (grid[r][c] > 1.0f) grid[r][c] = 1.0f;
                }
            }
        }
    }

    for (int r = 0; r < params.rows; r++) {
        for (int c = 0; c < params.columns; c++) {
            float height = grid[r][c];

            //bouncy <0
            // Rock (Value <0.07)
			// grass psila >=0.07

            if (height < 0.0f) {
                typeGrid[r][c] = -1.0f; // NEW: Bouncy Area
            }
            else if (height < 0.07f) {
                typeGrid[r][c] = 1.0f;  // Rock Area
            }
            else {
                typeGrid[r][c] = 0.0f;  // Grass Area
            }
        }
    }

    auto tempGrid = typeGrid;
    for (int r = 1; r < params.rows - 1; r++) {
        for (int c = 1; c < params.columns - 1; c++) {
            float sum = 0.0f;
            for (int ir = -1; ir <= 1; ir++)
                for (int ic = -1; ic <= 1; ic++)
                    sum += tempGrid[r + ir][c + ic];
            typeGrid[r][c] = sum / 9.0f;
        }
    }

    data.grid = grid;
    data.typeGrid = typeGrid;

    int numQuads = (params.rows - 1) * (params.columns - 1);
    data.v.reserve(numQuads * 6);
    data.uv.reserve(numQuads * 6);
    data.n.reserve(numQuads * 6);

    for (int i = 0; i < params.rows - 1; i++) {
        for (int j = 0; j < params.columns - 1; j++) {
            auto addVert = [&](int r, int c) {
                float h = grid[r][c];
                data.v.push_back(vec3(-0.5f + (float)c / (params.columns - 1), h, -0.5f + (float)r / (params.rows - 1)));
                data.uv.push_back(vec2((float)c / (params.columns - 1), (float)r / (params.rows - 1)));
                data.n.push_back(vec3(0, 1, 0));
                };
            addVert(i, j); addVert(i + 1, j); addVert(i, j + 1);
            addVert(i + 1, j); addVert(i + 1, j + 1); addVert(i, j + 1);
        }
    }
    return data;
}

mat4 Heightmap::returnplaneMatrix() {
    return scale(mat4(), vec3(scalar, scalarY, scalar));
}

float Heightmap::getHeightAt(float worldX, float worldZ) {
    float localX = (worldX - position.x) / scalar;
    float localZ = (worldZ - position.z) / scalar;
    float u = localX + 0.5f;
    float v = localZ + 0.5f;
    if (u < 0.0f || u >= 1.0f || v < 0.0f || v >= 1.0f) return -99999.0f;

    float r_f = v * (rows - 1);
    float c_f = u * (cols - 1);
    int r = (int)r_f;
    int c = (int)c_f;
    if (r < 0) r = 0; if (r >= rows - 1) r = rows - 2;
    if (c < 0) c = 0; if (c >= cols - 1) c = cols - 2;

    // Digrammikh parembolh
    float h00 = heightGrid[r][c];     float h10 = heightGrid[r + 1][c];
    float h01 = heightGrid[r][c + 1]; float h11 = heightGrid[r + 1][c + 1];
    float percentU = c_f - c; float percentV = r_f - r;
    float hTop = h00 * (1.0f - percentU) + h01 * percentU;
    float hBot = h10 * (1.0f - percentU) + h11 * percentU;
    return (hTop * (1.0f - percentV) + hBot * percentV) * scalarY + position.y;
}

float Heightmap::getGroundTypeAt(float worldX, float worldZ) {
    float localX = (worldX - position.x) / scalar;
    float localZ = (worldZ - position.z) / scalar;
    float u = localX + 0.5f;
    float v = localZ + 0.5f;

    if (u < 0.0f || u >= 1.0f || v < 0.0f || v >= 1.0f) return 0.0f; 

    float r_f = v * (rows - 1);
    float c_f = u * (cols - 1);
    int r = (int)r_f;
    int c = (int)c_f;
    if (r < 0) r = 0; if (r >= rows - 1) r = rows - 2;
    if (c < 0) c = 0; if (c >= cols - 1) c = cols - 2;

    // Digrammikh parembolh
    float t00 = typeGrid[r][c];     float t10 = typeGrid[r + 1][c];
    float t01 = typeGrid[r][c + 1]; float t11 = typeGrid[r + 1][c + 1];

    float percentU = c_f - c;
    float percentV = r_f - r;

    float tTop = t00 * (1.0f - percentU) + t01 * percentU;
    float tBot = t10 * (1.0f - percentU) + t11 * percentU;

    return tTop * (1.0f - percentV) + tBot * percentV;
}

vec3 Heightmap::getNormalAt(float worldX, float worldZ) {
    float localX = (worldX - position.x) / scalar;
    float localZ = (worldZ - position.z) / scalar;
    int r = (int)((localZ + 0.5f) * (rows - 1));
    int c = (int)((localX + 0.5f) * (cols - 1));
    if (r < 1) r = 1; if (r >= rows - 1) r = rows - 2;
    if (c < 1) c = 1; if (c >= cols - 1) c = cols - 2;
    float hL = heightGrid[r][c - 1]; float hR = heightGrid[r][c + 1];
    float hD = heightGrid[r - 1][c]; float hU = heightGrid[r + 1][c];
    float unitStep = scalar / (float)(cols - 1);
    vec3 tangentX(2.0f * unitStep, (hR - hL) * scalarY, 0.0f);
    vec3 tangentZ(0.0f, (hU - hD) * scalarY, 2.0f * unitStep);
    return normalize(cross(tangentZ, tangentX));
}