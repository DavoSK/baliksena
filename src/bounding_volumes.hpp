#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtx/quaternion.hpp>

struct Sphere {
	glm::vec3 center{ 0.f, 0.f, 0.f };
	float radius{ 0.f };
};