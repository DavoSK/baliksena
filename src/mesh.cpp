#include "mesh.hpp"
#include "material.hpp"

/*
    FaceGroup
*/

void FaceGroup::render() const {    
    if(mMaterial != nullptr)
        mMaterial->bind();

    Renderer::bindBuffers();
    Renderer::applyUniforms();
    Renderer::draw(mOffset, static_cast<int>(mIndices.size()), 1);
}

/*
    Mesh
*/

void Mesh::render() {
    Frame::render();

    if(mVertices.empty())
        return;

    for (auto& faceGroup : mFaceGroups) {
        faceGroup->render();
    }
}