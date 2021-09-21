#pragma once
#include <glm/glm.hpp>

/*
enum MovementFlags : uint64_t {
    FORWARD = (1 << 0),
    BACKWARD = (1 << 1),
    LEFT = (1 << 2),
    RIGHT = (1 << 3)
};
*/

struct Input {
	float mouseDeltaX;
	float mouseDeltaY;
	float mouseX;
	float mouseY;
    glm::vec3 movementDir;
};