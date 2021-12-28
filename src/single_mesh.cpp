#include  "single_mesh.hpp"
#include "renderer.hpp"
#include "logger.hpp"
#include "renderer.hpp"
#include "animator.hpp"
#include "glm/gtx/string_cast.hpp"

SingleMesh::SingleMesh() {
    mAnimator = std::make_shared<Animator>(this);
    mAnimator->open("!!!Skakani.5DS");
}

void SingleMesh::render() {
    mAnimator->update();

    std::vector<glm::mat4> bones{};
    for(auto& boneDef : mBones) {
        bones.push_back(boneDef.mInverseTransform);
    }

    forEach<Joint>([&](Joint* joint) {
        for(size_t i = 0; i < bones.size(); i++) {
            if(i == joint->getBoneId()) {
                bones[i] = glm::inverse(this->getWorldMatrix()) * joint->getWorldMatrix() * bones[i];
            }
        }        
    }, this);

    Renderer::setBones(bones);
    Mesh::render();
    Renderer::setBones({});
}