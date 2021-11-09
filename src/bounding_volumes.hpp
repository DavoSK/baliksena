#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtx/quaternion.hpp>

struct Plan {
	glm::vec3 normal = { 0.f, 1.f, 0.f }; // unit vector
	float     distance = 0.f;        // Distance with origin

	Plan() = default;

	Plan(const glm::vec3& p1, const glm::vec3& norm)
		: normal(glm::normalize(norm)),
		distance(glm::dot(normal, p1))
	{}

	float getSignedDistanceToPlan(const glm::vec3& point) const {
		return glm::dot(normal, point) - distance;
	}
};

struct Sphere {
	glm::vec3 center{ 0.f, 0.f, 0.f };
	float radius{ 0.f };
};

struct Frustum {
	Plan topFace;
	Plan bottomFace;

	Plan rightFace;
	Plan leftFace;

	Plan farFace;
	Plan nearFace;

    bool isSphereInFrustum(const Sphere& sphere) {
		auto isOnOrForwardPlanSphere = [&sphere](const Plan& plan) -> bool {
			return plan.getSignedDistanceToPlan(sphere.center) > -sphere.radius;
		};

        return (isOnOrForwardPlanSphere(leftFace) 	&&
			isOnOrForwardPlanSphere(rightFace) 		&&
			isOnOrForwardPlanSphere(topFace) 		&&
			isOnOrForwardPlanSphere(bottomFace) 	&&
			isOnOrForwardPlanSphere(nearFace) 		&&
			isOnOrForwardPlanSphere(farFace));
    }
};

/*
	//Get global scale thanks to our transform
	const glm::vec3 globalScale = frame.getGlobalScale();

	//Get our global center with process it with the global model matrix of our transform
	const glm::vec3 globalCenter{ frame.getWorldMatrix() * glm::vec4(center, 1.f) };

	//To wrap correctly our shape, we need the maximum scale scalar.
	const float maxScale = std::max(std::max(globalScale.x, globalScale.y), globalScale.z);

	//Max scale is assuming for the diameter. So, we need the half to apply it to our radius
	Sphere globalSphere(globalCenter, radius * (maxScale * 0.5f));
*/
