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
    verticalAngle -= mouseSpeed * float(height / 2 - yPos);

    if (verticalAngle > 1.5f) verticalAngle = 1.5f;
    if (verticalAngle < -1.5f) verticalAngle = -1.5f;

    float camDist = 23.1214f * snail->radius;

    vec3 offset;
    offset.x = camDist * cos(verticalAngle) * sin(horizontalAngle);
    offset.y = camDist * sin(verticalAngle);
    offset.z = camDist * cos(verticalAngle) * cos(horizontalAngle);

    position = snail->x + offset;

    vec3 lookTarget = snail->x;

    vec3 up = vec3(0, 1, 0);

    projectionMatrix = perspective(radians(FoV), (float)width / (float)height, 0.1f, 200.41f * 2 *snail->radius);
    viewMatrix = lookAt(position, lookTarget, up);

    lastTime = currentTime;
}