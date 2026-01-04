#include "heightmap.h"
#include <random>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

// 1. Public Constructor delegates to Private Constructor
Heightmap::Heightmap(const HillAlgorithmParameters& params)
    : Heightmap(generate(params)) 
{
    this->scalar = params.scalar;
    this->scalarY = params.scalarY;
    this->rows = params.rows;
    this->cols = params.columns;
    this->position = glm::vec3(0.0f, 0.0f, 0.0f);

}

// 2. Private Constructor passes data to Drawable
Heightmap::Heightmap(const MeshData& data)
    : Drawable(data.v, data.uv, data.n)
{
    rows = 0;
    cols = 0;
    scalar = 0;
    scalarY = 0;
    this->heightGrid = data.grid;
}



// 3. The Logic (Your implementation wrapped in a static function)
Heightmap::MeshData Heightmap::generate(const HillAlgorithmParameters& params)
{
    MeshData data;

    // --- A. Generate Height Grid ---
	std::vector<std::vector<float>> grid(params.rows, std::vector<float>(params.columns, 0.0f)); // grid to contain the height values
	std::random_device rd;          // seed for random number generator
	std::mt19937 generator(rd());   // Mersenne Twister RNG (best psuedorandom generator)
	std::uniform_int_distribution<int> rDist(params.hillRadiusMin, params.hillRadiusMax); //distribution for hill radius (integer)
	std::uniform_real_distribution<float> hDist(params.hillMinHeight, params.hillMaxHeight); //distribution for hill height (float)
    std::uniform_int_distribution<int> rowDist(0, params.rows - 1);     //center of circle must be in grid
	std::uniform_int_distribution<int> colDist(0, params.columns - 1);  //center of circle must be in grid

    for (int i = 0; i < params.numHills; i++) {
		int cR = rowDist(generator); // random selection of center row
		int cC = colDist(generator); // random selection of center column
		int rad = rDist(generator);  // random selection of radius from the center
		float hillH = hDist(generator); // random selection of hill height

        // Iterate over square bounding box
		for (int r = cR - rad; r < cR + rad; r++) { // for rows inside the radius
            for (int c = cC - rad; c < cC + rad; c++) { // for columns inside the radius
                if (r < 0 || r >= params.rows || c < 0 || c >= params.columns) continue; 

                float r2 = float(rad * rad); 
                float dx = float(cC - c);
                float dy = float(cR - r);
				float hVal = (r2 - dx * dx - dy * dy) / 5; // pyramid formula /5 to flatten hills a bit

                if (hVal > 0.0f) {
                    grid[r][c] += hillH * (hVal / r2);
                    if (grid[r][c] > 1.0f) grid[r][c] = 1.0f;
                }
            }
        }
    }
    data.grid = grid;

    // --- B. Convert to Triangle Soup (Drawable format) ---
    // Resize vectors to fit all triangles
    int numQuads = (params.rows - 1) * (params.columns - 1);
	data.v.reserve(numQuads * 6); // gia kathe tetragwno 2 trigwna = 6 shmeia
    data.uv.reserve(numQuads * 6); 
    data.n.reserve(numQuads * 6);

    for (int i = 0; i < params.rows - 1; i++) { 
        for (int j = 0; j < params.columns - 1; j++) {

            // Helper to make a vertex
            auto addVert = [&](int r, int c) {
                float h = grid[r][c];
                // Pos
                data.v.push_back(vec3(
                    -0.5f + (float)c / (params.columns - 1),
                    h,
                    -0.5f + (float)r / (params.rows - 1)
                ));
                // UV
                data.uv.push_back(vec2(
                    (float)c / (params.columns - 1),
                    (float)r / (params.rows - 1)
                ));
                // Normal (Simplification: Up vector for now, or recalculate like previous answer)
                data.n.push_back(vec3(0, 1, 0));
                };

            // Triangle 1
            addVert(i, j);
            addVert(i + 1, j);
            addVert(i, j + 1);

            // Triangle 2
            addVert(i + 1, j);
            addVert(i + 1, j + 1);
            addVert(i, j + 1);
        }
    }

    // NOTE: If you want smooth normals, you must calculate them here 
    // inside the static function before returning 'data'.

    return data;
}


glm::mat4 Heightmap::returnplaneMatrix() {

    mat4 planeMatrix = scale(mat4(), vec3(scalar, scalarY, scalar));
    return planeMatrix;
}


float Heightmap::getHeightAt(float worldX, float worldZ) {
    float localX = (worldX - position.x) / scalar;
    float localZ = (worldZ - position.z) / scalar;

    // 2. Map to 0..1 range (UV space)
    float u = localX + 0.5f;
    float v = localZ + 0.5f;

    // 3. Safety Check: Is coordinates off the map?
    if (u < 0.0f || u >= 1.0f || v < 0.0f || v >= 1.0f) {
        return -99999.0f; // Return a "pit" value
    }

    // 4. Convert to Grid Indices
    // In generate(): Z mapped to Row (r), X mapped to Column (c)
    float r_f = v * (rows - 1);
    float c_f = u * (cols - 1);

    int r = (int)r_f;
    int c = (int)c_f;

    // Clamp indices to be safe
    if (r < 0) r = 0; if (r >= rows - 1) r = rows - 2;
    if (c < 0) c = 0; if (c >= cols - 1) c = cols - 2;

    // 5. Bilinear Interpolation (Smooth height calculation)
    float h00 = heightGrid[r][c];
    float h10 = heightGrid[r + 1][c];
    float h01 = heightGrid[r][c + 1];
    float h11 = heightGrid[r + 1][c + 1];

    float percentU = c_f - c;
    float percentV = r_f - r;

    // Interpolate Top Edge and Bottom Edge
    float hTop = h00 * (1.0f - percentU) + h01 * percentU;
    float hBot = h10 * (1.0f - percentU) + h11 * percentU;

    // Interpolate Vertical
    float finalHeight = hTop * (1.0f - percentV) + hBot * percentV;

    // 6. Scale back to world units
    return finalHeight * scalarY + position.y;
}

vec3 Heightmap::getNormalAt(float worldX, float worldZ) {
    // 1. Convert World Coords to Grid Indices (Same logic as getHeightAt)
    float localX = (worldX - position.x) / scalar;
    float localZ = (worldZ - position.z) / scalar;

    // Grid indices
    int r = (int)((localZ + 0.5f) * (rows - 1));
    int c = (int)((localX + 0.5f) * (cols - 1));

    // Clamp to ensure we don't go out of bounds
    // We need neighbors, so we clamp 1 unit away from edges
    if (r < 1) r = 1; if (r >= rows - 1) r = rows - 2;
    if (c < 1) c = 1; if (c >= cols - 1) c = cols - 2;

    // 2. Get Heights of Neighbors
    // hL = Left, hR = Right, hD = Down, hU = Up
    float hL = heightGrid[r][c - 1];
    float hR = heightGrid[r][c + 1];
    float hD = heightGrid[r - 1][c];
    float hU = heightGrid[r + 1][c];

    // 3. Calculate Tangent Vectors
    // The "step" size between grid points in normalized space is 1.0/cols
    // But we need it in World Space dimensions.
    // Width of one grid square = scalar / (cols-1)
    float unitStep = scalar / (float)(cols - 1);

    // Tangent X (Slope from Left to Right)
    // Vector points (2 units right, deltaHeight, 0)
    vec3 tangentX(2.0f * unitStep, (hR - hL) * scalarY, 0.0f);

    // Tangent Z (Slope from Down to Up)
    // Vector points (0, deltaHeight, 2 units down)
    vec3 tangentZ(0.0f, (hU - hD) * scalarY, 2.0f * unitStep);

    // 4. Calculate Normal using Cross Product
    // The normal is perpendicular to both tangents
    vec3 normal = glm::normalize(cross(tangentZ, tangentX));

    return normal;
}