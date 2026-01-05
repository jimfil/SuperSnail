#include <glfw3.h>
#include <iostream>
#include <math.h>
#include <glm/gtc/matrix_transform.hpp>
#include "light.h"

using namespace glm;

Light::Light(GLFWwindow* window, 
             glm::vec4 init_La,
             glm::vec4 init_Ld,
             glm::vec4 init_Ls,
             glm::vec3 init_position) : window(window) {
    La = init_La;
    Ld = init_Ld;
    Ls = init_Ls;
    lightPosition_worldspace = init_position;

    // setting near and far plane affects the detail of the shadow
    nearPlane = 1.0;
    farPlane = 300.0;

    direction = normalize(targetPosition - lightPosition_worldspace);

    lightSpeed = 0.1f;
    targetPosition = glm::vec3(0.0, 0.0, -5.0);

    float size = 300.0f;
    projectionMatrix = glm::ortho(-size, size, -size, size, nearPlane, farPlane);
}


void Light::update(vec3 centerPosition) {

    targetPosition = centerPosition;
    vec3 sunOffset = vec3(0, 140.0f, 0);
    lightPosition_worldspace = targetPosition + sunOffset;

    direction = normalize(targetPosition - lightPosition_worldspace);

    
    float x = direction.x;
    float y = direction.y;
    float z = direction.z;

    
    float horizontalAngle;
    if (z > 0.0) horizontalAngle = atan(x/z);
    else if (z < 0.0) horizontalAngle = atan(x/z) + 3.1415f;
    else horizontalAngle = 3.1415f / 2.0f;

    // Right vector
    vec3 right(
        sin(horizontalAngle - 3.14f / 2.0f),
        0,
        cos(horizontalAngle - 3.14f / 2.0f)
    );

    // Up vector
    vec3 up = cross(right, direction);
   
    viewMatrix = lookAt(
        lightPosition_worldspace,
        targetPosition,
        up 
    );
    //*/

}


mat4 Light::lightVP() {
    return projectionMatrix * viewMatrix;
}