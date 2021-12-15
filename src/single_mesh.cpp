#include  "single_mesh.hpp"
#include "renderer.hpp"
#include "logger.hpp"

void SingleMesh::render() {
    // forEach<Joint>([&](Joint* joint) {
    //     for(auto& bone : mBones) {
    //         if(bone.mBoneID == joint->getBoneId()) {
    //             bones[joint->getBoneId()] = joint->getMatrix();
    //             break;
    //         }
    //     }        
    // }, this);

    std::vector<glm::mat4> bones{};
    for(const auto& boneDef : mBones) {
        glm::mat4 inverseTransformMatrix = glm::inverse(boneDef.mInverseTransform);
        bones.push_back(glm::mat3(inverseTransformMatrix));
    }

    
    Renderer::setBones(bones);
    Mesh::render();
    Renderer::setBones({});
}