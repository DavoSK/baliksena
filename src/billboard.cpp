#include "billboard.hpp"
#include "app.hpp"
#include "scene.hpp"
#include "camera.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/glm.hpp>

void Billboard::render() {
    //NOTE: change matrix perpendicular to camera
    /*const auto& localMatrix = getMatrix();
    const auto& worldPosition = glm::vec3(getWorldMatrix()[3]);

    auto* scene = App::get()->getScene();
    if(const auto& cam = scene->getActiveCamera().lock()) {
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(localMatrix, scale, rotation, translation, skew,perspective);

        auto camPos = cam->getViewMatrix()[3];
        auto dir = glm::vec3(camPos) - worldPosition;
        float angle = atan2f(dir.x, dir.z);

        const auto translation_ = glm::translate(glm::mat4(1.f), translation);
        const auto scale_ = glm::scale(glm::mat4(1.f), scale);
        const auto rot = glm::rotate(glm::toMat4(rotation), angle, glm::vec3(0.0f, 1.0f, 0.0f));
        const auto world = translation_ * rot * scale_;
        setMatrix(world);
    }*/

    Mesh::render();
}