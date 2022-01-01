//
// Created by david on 24. 4. 2021.
//

#ifndef EXPERIMENT_CAMERA_H
#define EXPERIMENT_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr float SPEED_FAST = 80.0f;
constexpr float YAW = -90.0f;
constexpr float PITCH = 0.0f;
constexpr float SPEED = 0.05f;
constexpr float SENSITIVITY = 0.4f;
constexpr float FOV = 65.0f;

#include "bounding_volumes.hpp"
#include "frame.hpp"
#include "frustum_culling.h"

class Camera : public Frame {
public:
    [[nodiscard]] constexpr FrameType getFrameType() const override { return FrameType::Camera; }

    // NOTE: camera attributes
    glm::vec3 Front = {0.0f, 0.0f, 0.0f};
    glm::vec3 Up = {0.0f, 0.0f, 0.0f};
    glm::vec3 Right = {0.0f, 0.0f, 0.0f};
    glm::vec3 WorldUp = {0.0f, 1.0f, 0.0f};

    // NOTE: euler angles
    float Yaw = YAW;
    float Pitch = PITCH;

    // NOTE: camera options
    float MovementSpeed = SPEED;
    float MouseSensitivity = SENSITIVITY;
    float Fov = FOV;
    float Aspect = 0.0f;
    float Near = 0.0f;
    float Far = 0.0f;

    Camera() 
    {
        updateCameraVectors();
    }

    glm::mat4 createProjMatrix(int width, int height, float fov = 65.0f, float near = 0.01f, float far = 500.0f);
    void setProjMatrix(const glm::mat4& proj) { mProjMatrix = proj; }

    [[nodiscard]] const glm::mat4& getProjMatrix() const { return mProjMatrix; }
    [[nodiscard]] const glm::mat4& getSkyboxProjMatrix() const { return mProjSkyboxMatrix; }
    [[nodiscard]] float getFOV() const { return Fov; }
    [[nodiscard]] float getPitch() const { return Pitch; }
    [[nodiscard]] float getYaw() const { return Yaw; }
    [[nodiscard]] Frustum& getFrustum() { return mFrustum; }

    void setPosDelta(const glm::vec3& posDelta) { mPosDelta = posDelta; }
    void setDirDelta(const glm::vec2& dirDelta) { mDirDelta = dirDelta; }
    void update(float deltaTime);
    void debugRender() override;
    void updateFrustum();
    void processInput();
    void updateCameraVectors();
private:
    Frustum mFrustum{};
    glm::mat4 mViewMatrix;
    glm::mat4 mProjMatrix;
    glm::mat4 mProjSkyboxMatrix;
    glm::vec3 mPosDelta;
    glm::vec2 mDirDelta;
};

#endif  // EXPERIMENT_CAMERA_H
