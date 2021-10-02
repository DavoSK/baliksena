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
    Renderer::draw(mOffset, mIndicesCount, 1);
}

/*
    Mesh
*/

void Mesh::render() {
    Frame::render();
    if(mStatic || mVertices.empty()) return;

    Renderer::setModel(getWorldMatrix());

    for (auto& faceGroup : mFaceGroups) {
        faceGroup->render();
    }
}