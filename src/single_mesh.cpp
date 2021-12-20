#include  "single_mesh.hpp"
#include "renderer.hpp"
#include "logger.hpp"
#include "renderer.hpp"
#include "animator.hpp"

SingleMesh::SingleMesh() {
    mAnimator = std::make_shared<Animator>(this);
    mAnimator->open("!!!Skakani.5DS");
}

void SingleMesh::render() {
    
    mAnimator->update();
    

    std::vector<glm::mat4> bones{};
    for(auto& boneDef : mBones) {
        glm::mat4 inverseTransformMatrix = glm::inverse(boneDef.mInverseTransform);
        glm::mat4 mat = glm::mat3(inverseTransformMatrix);
        bones.push_back(mat);
    }

    auto getJointMatrix = [&](Joint* of) -> glm::mat4 {
        Joint* currentFrame = of;
        std::vector<glm::mat4> transforms;
     
        while(true) {
            if(currentFrame->getFrameType() != FrameType::Joint) {
                std::reverse(transforms.begin(), transforms.end());

                glm::mat4 res(1.0f);
                for(const auto& nextParent : transforms) {
                    res = nextParent * res;
                }

                return res;
            }

            transforms.push_back(currentFrame->getLocalMatrix());
            currentFrame = (Joint*)currentFrame->getOwner();
        }
    };

    forEach<Joint>([&](Joint* joint) {
        for(size_t i = 0; i < bones.size(); i++) {
            if(i == joint->getBoneId()) {
                auto jointBoneSpaceMatrix = getJointMatrix(joint);
                bones[i] = jointBoneSpaceMatrix * bones[i];
            }
        }        
    }, this);

    Renderer::setBones(bones);
    Mesh::render();
    Renderer::setBones({});
}