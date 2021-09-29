#include "mesh.hpp"
#include "material.hpp"

/*
    FaceGroup
*/

void FaceGroup::render() const {    
    if(mMaterial != nullptr)
        mMaterial->bind();

    Renderer::draw(mOffset, static_cast<int>(mIndices.size()), 1);
}

/*
    Mesh
*/

void Mesh::render() {
    Frame::render();

    if(mVertices.empty())
        return;

    Renderer::setModel(getWorldMatrix());

    for (auto& faceGroup : mFaceGroups) {
        faceGroup->render();
    }
}