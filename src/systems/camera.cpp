//
// Created by david on 21. 9. 2021.
//

#include "camera.h"
#include "renderer.hpp"

#include <components/input.hpp>
#include <components/camera.hpp>
#include <glm/gtc/matrix_transform.hpp>

void CameraSystem::add(flecs::world &ecs) {
    //NOTE: camera controller system
    ecs.system<Camera, Input>().each([&ecs](Camera &cam, Input &input) {
        //NOTE: process mouse movement
        glm::vec2 mouseDelta{input.mouseDeltaX * cam.MouseSensitivity,
                             input.mouseDeltaY * cam.MouseSensitivity};
        cam.Yaw -= mouseDelta.x;
        cam.Pitch -= mouseDelta.y;

        if (cam.Pitch > 89.0f) cam.Pitch = 89.0f;
        if (cam.Pitch < -89.0f) cam.Pitch = -89.0f;

        //NOTE: process keyboard
        const auto velocity = cam.MovementSpeed * ecs.delta_time();
        cam.Position += cam.Front * input.movementDir * velocity;

        //NOTE: calculate vectors & matrices
        const glm::vec3 front = {cos(glm::radians(cam.Yaw)) * cos(glm::radians(cam.Pitch)),
                                 sin(glm::radians(cam.Pitch)),
                                 sin(glm::radians(cam.Yaw)) * cos(glm::radians(cam.Pitch))};

        cam.Front = glm::normalize(front);
        cam.Right = glm::normalize(glm::cross(cam.Front, glm::vec3(0.0f, 1.0f, 0.0f)));
        cam.Up = glm::normalize(glm::cross(cam.Right, cam.Front));
        cam.ViewMatrix = glm::lookAt(cam.Position, cam.Position + cam.Front, cam.Up);
    });
}
