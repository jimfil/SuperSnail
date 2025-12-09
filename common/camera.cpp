#include <glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "camera.h"
#include "../ergasia/sourcefiles/Snail.h"

using namespace glm;

Camera::Camera(GLFWwindow* window) : window(window) {
    position = vec3(0, 0, 5);
    horizontalAngle = 3.14f;
    verticalAngle = 0.0f;
    FoV = 45.0f;
    speed = 10.0f;
    mouseSpeed = 0.001f;
    fovSpeed = 2.0f;
}

void Camera::update(Snail* snail) {
    static double lastTime = glfwGetTime();
    double currentTime = glfwGetTime();
    float deltaTime = float(currentTime - lastTime);

    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    glfwSetCursorPos(window, width / 2, height / 2);

    horizontalAngle += mouseSpeed * float(width / 2 - xPos);
    verticalAngle += mouseSpeed * float(height / 2 - yPos);

    if (verticalAngle > 1.48f) verticalAngle = 1.48f;
    if (verticalAngle < -1.48f) verticalAngle = -1.48f;

    if (horizontalAngle > 4.5f) horizontalAngle = 4.5f;
    if (horizontalAngle < 1.5f) horizontalAngle = 1.5f;

    vec3 lookDirection(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );

    float camHeight = 20.0f; 
    float camDist = 40.0f;   
    
    vec3 localOffset = vec3(0.0f, camHeight, -camDist);
    vec3 worldOffset = snail->q * localOffset;
    
    position = snail->x + worldOffset;
    vec3 lookTarget = snail->x + (lookDirection * 5.0f);
    vec3 snailUp = snail->q * vec3(0, 1, 0);

    projectionMatrix = perspective(radians(FoV), (float)width/(float)height, 0.1f, 300.0f);
    viewMatrix = lookAt(position, lookTarget, snailUp);

    lastTime = currentTime;
}