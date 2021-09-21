//
// Created by david on 24. 4. 2021.
//

#include "camera.h"
#include "app.hpp"
#include "input.hpp"

glm::mat4 Camera::createProjMatrix(int width, int height) {
    mProjMatrix = glm::perspectiveLH(glm::radians(getFOV()), (float)width / (float)height, 0.01f, 2000.0f);
    return mProjMatrix;
}

glm::mat4 Camera::getViewMatrix() {
    mViewMatrix = glm::lookAt(Position, Position + Front, Up);
    return mViewMatrix;
}

void Camera::update(float deltaTime) {
    const auto velocity = MovementSpeed * deltaTime;
    Position += Front * (mPosDelta * velocity);

    Yaw -= mDirDelta.x * MouseSensitivity;
    Pitch -= mDirDelta.y * MouseSensitivity;
    printf("%f %f, %f %f %f\n", mDirDelta.x, mDirDelta.y, Position.x, Position.y, Position.z);

    if (Pitch > 89.0f) Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;

    updateCameraVectors();
}

void Camera::updateCameraVectors() {
    const glm::vec3 front = {cos(glm::radians(Yaw)) * cos(glm::radians(Pitch)),
                             sin(glm::radians(Pitch)),
                             sin(glm::radians(Yaw)) * cos(glm::radians(Pitch))};
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}
