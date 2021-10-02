#include "billboard.hpp"
#include "app.hpp"
#include "scene.hpp"
#include "camera.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/glm.hpp>

void Billboard::render() {
    
    //NOTE: change matrix perpendicular to camera
    auto worldMatrix = getMatrix();
    const auto& worldPosition = glm::vec3(worldMatrix[3]);

    auto* scene = App::get()->getScene();
    if(const auto& cam = scene->getActiveCamera().lock()) {
        
        //auto dir = glm::normalize(worldPosition - cam->Position);
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(worldMatrix, scale, rotation, translation, skew,perspective);

        auto lookPos = glm::lookAt(worldPosition,  cam->Position, cam->WorldUp);

        const auto translation_ = glm::translate(glm::mat4(1.f), translation);
        const auto scale_ = glm::scale(glm::mat4(1.f), scale);
        const auto rot = lookPos;
        const auto world = translation_ * rot * scale_;
        setMatrix(world);
    }

    Mesh::render();
}