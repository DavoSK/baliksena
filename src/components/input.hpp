#pragma once
#include <glm/glm.hpp>

struct Input {
	float mouseDeltaX;
	float mouseDeltaY;
	float mouseX;
	float mouseY;
    glm::vec3 movementDir;
};