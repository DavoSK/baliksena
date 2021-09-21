//
// Created by david on 21. 9. 2021.
//

#include "camera.h"

#include <components/input.hpp>
#include <components/camera.hpp>

void CameraSystem::add(flecs::world &ecs) {
    //NOTE: camera controller system
    ecs.system<Camera, Input>().each([&ecs](Camera &camComp, Input &inputComp) {
        //NOTE: process mouse movement
        glm::vec2 mouseDelta{inputComp.mouseDeltaX * camComp.MouseSensitivity,
                             inputComp.mouseDeltaY * camComp.MouseSensitivity};
        camComp.Yaw -= mouseDelta.x;
        camComp.Pitch -= mouseDelta.y;

        if (camComp.Pitch > 89.0f) camComp.Pitch = 89.0f;
        if (camComp.Pitch < -89.0f) camComp.Pitch = -89.0f;

        //NOTE: process keyboard
        const auto velocity = camComp.MovementSpeed * ecs.delta_time();
        camComp.Position += camComp.Front * inputComp.movementDir * velocity;

        //NOTE: calculate vectors & matrices
        const glm::vec3 front = {cos(glm::radians(camComp.Yaw)) * cos(glm::radians(camComp.Pitch)),
                                 sin(glm::radians(camComp.Pitch)),
                                 sin(glm::radians(camComp.Yaw)) * cos(glm::radians(camComp.Pitch))};

        camComp.Front = glm::normalize(front);
        camComp.Right = glm::normalize(glm::cross(camComp.Front, glm::vec3(0.0f, 1.0f, 0.0f)));
        camComp.Up = glm::normalize(glm::cross(camComp.Right, camComp.Front));
    });
}
