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

    // 1. Update angles based on mouse movement
    horizontalAngle += mouseSpeed * float(width / 2 - xPos);
    verticalAngle -= mouseSpeed * float(height / 2 - yPos);

    // 2. Clamp vertical angle to prevent the camera from flipping over the top/bottom
    if (verticalAngle > 1.5f) verticalAngle = 1.5f;
    if (verticalAngle < -1.5f) verticalAngle = -1.5f;

    // 3. Spherical Coordinates to Cartesian Mapping
    // This calculates the offset from the snail based on your mouse angles
    float camDist = 40.0f;

    vec3 offset;
    offset.x = camDist * cos(verticalAngle) * sin(horizontalAngle);
    offset.y = camDist * sin(verticalAngle);
    offset.z = camDist * cos(verticalAngle) * cos(horizontalAngle);

    // 4. Set Camera Position and Target
    // Position is the snail's center plus the calculated orbit offset
    position = snail->x + offset;

    // Target is simply the snail itself
    vec3 lookTarget = snail->x;

    // 5. Fixed "Up" vector usually works best for orbit cameras to prevent spinning
    vec3 up = vec3(0, 1, 0);

    projectionMatrix = perspective(radians(FoV), (float)width / (float)height, 0.1f, 300.0f);
    viewMatrix = lookAt(position, lookTarget, up);

    lastTime = currentTime;
}