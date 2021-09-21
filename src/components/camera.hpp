#pragma once
#include <glm/glm.hpp>

struct Camera  {
    // NOTE: camera attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // NOTE: euler angles
    float Yaw;
    float Pitch;

    // NOTE: camera options
    float MovementSpeed = 10.0f;
    float MouseSensitivity = 0.1f;
    float Zoom = 65.0f;

    glm::mat4 ViewMatrix;
    glm::mat4 ProjMatrix;
};