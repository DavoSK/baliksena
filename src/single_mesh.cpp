#include  "single_mesh.hpp"
#include "renderer.hpp"
#include "logger.hpp"

void SingleMesh::render() {

    std::vector<glm::mat4> bones{};
    bones.resize(mBones.size());

    forEach<Joint>([&](Joint* joint) {
        for(auto& bone : mBones) {
            if(bone.mBoneID == joint->getBoneId()) {
                bones[joint->getBoneId()] = joint->getMatrix();
                break;
            }
            
        }

      
        
    }, this);

    

    Logger::get().info("pog {}", bones.size());

    Renderer::setBones(bones);
    Mesh::render();

    bones.clear();
    Renderer::setBones(bones);
}