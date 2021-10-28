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
constexpr float ZOOM = 65.0f;

class Camera {
public:
    // NOTE: camera attributes
    glm::vec3 Position = {0.0f, 0.0f, 0.0f};
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
    float Zoom = ZOOM;
    float Aspect = 0.0f;
    float Near = 0.0f;
    float Far = 0.0f;

    Camera() {
        updateCameraVectors();
    }

    glm::mat4 createProjMatrix(int width, int height);
    glm::mat4 getViewMatrix();

    [[nodiscard]] const glm::mat4& getProjMatrix() const { return mProjMatrix; }
    [[nodiscard]] const glm::mat4& getSkyboxProjMatrix() const { return mProjSkyboxMatrix; }
    [[nodiscard]] const glm::vec3& getPos() const { return Position; }
    [[nodiscard]] const glm::vec3& getRight() const { return Right; }
    [[nodiscard]] float getFOV() const { return Zoom; }
    [[nodiscard]] float getPitch() const { return Pitch; }
    [[nodiscard]] float getYaw() const { return Yaw; }

    void setPosDelta(const glm::vec3& posDelta) { mPosDelta = posDelta; }
    void setDirDelta(const glm::vec2& dirDelta) { mDirDelta = dirDelta; }
    void update(float deltaTime);
    void processInput();
private:
    void updateCameraVectors();
    glm::mat4 mViewMatrix;
    glm::mat4 mProjMatrix;
    glm::mat4 mProjSkyboxMatrix;
    glm::vec3 mPosDelta;
    glm::vec2 mDirDelta;
};

#endif  // EXPERIMENT_CAMERA_H
