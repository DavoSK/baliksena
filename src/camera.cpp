//
// Created by david on 24. 4. 2021.
//

#include "camera.h"
#include "input.hpp"

glm::mat4 Camera::createProjMatrix(int width, int height) {
    Aspect = (float)width / (float)height;
    Near = 0.1f;
    Far = 1000.0f;
    mProjMatrix = glm::perspectiveLH(glm::radians(getFOV()), Aspect, Near, Far);
    mProjSkyboxMatrix = glm::perspectiveLH(glm::radians(Fov), (float)width / (float)height, 0.1f, 2000.0f);

    updateCameraVectors();
    updateFrustum();

    return mProjMatrix;
}

glm::mat4 Camera::getViewMatrix() {
    mViewMatrix = glm::lookAtLH(Position, Position + Front, Up);
    return mViewMatrix;
}

void Camera::update(float deltaTime) {
    const auto velocity = MovementSpeed * deltaTime;
    Position += mPosDelta.x * (Front * velocity);
    Position += mPosDelta.y * (Up * velocity);
    Position += mPosDelta.z * (Right * velocity);

    Yaw -= mDirDelta.x * MouseSensitivity;
    Pitch -= mDirDelta.y * MouseSensitivity;

    if (Pitch > 89.0f) Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;

    updateCameraVectors();
    updateFrustum();
}

void Camera::updateFrustum() {
	const float halfVSide = Far * tanf(glm::radians(Fov) * .5f);
	const float halfHSide = halfVSide * Aspect;
	const glm::vec3 frontMultFar = Far * Front;

	mFrustum.nearFace = { Position + Near * Front, Front };
	mFrustum.farFace = { Position + frontMultFar, -Front };
	mFrustum.rightFace = { Position, glm::cross(Up, frontMultFar + Right * halfHSide) };
	mFrustum.leftFace = { Position, glm::cross(frontMultFar - Right * halfHSide, Up) };
	mFrustum.topFace = { Position, glm::cross(Right, frontMultFar - Up * halfVSide) };
	mFrustum.bottomFace = { Position, glm::cross(frontMultFar + Up * halfVSide, Right) };
}

void Camera::updateCameraVectors() {
    const glm::vec3 front = {cos(glm::radians(Yaw)) * cos(glm::radians(Pitch)),
                             sin(glm::radians(Pitch)),
                             sin(glm::radians(Yaw)) * cos(glm::radians(Pitch))};
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}
