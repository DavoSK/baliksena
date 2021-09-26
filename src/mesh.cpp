#include "mesh.hpp"
#include "texture.hpp"

/*
    FaceGroup
*/

FaceGroup::~FaceGroup() {
    Renderer::destroyBuffer(mIndexBuffer);
}

void FaceGroup::render() const {
    Renderer::bindIndexBuffer(mIndexBuffer);
    mMaterial->bind(0);
    Renderer::draw(0, static_cast<int>(mIndices.size()), 1);
}

void FaceGroup::init() {
    mIndexBuffer = Renderer::createIndexBuffer(mIndices);
}

/*
    Mesh
*/

Mesh::~Mesh() {
    Renderer::destroyBuffer(mVertexBuffer);
}

void Mesh::render() {
    Frame::render();
    Renderer::bindVertexBuffer(mVertexBuffer);
    Renderer::setModel(getWorldMatrix());

    for (auto& faceGroup : mFaceGroups) {
        faceGroup->render();
    }
}

void Mesh::init() {
    mVertexBuffer = Renderer::createVertexBuffer(mVertices);

    for (auto& faceGroup : mFaceGroups) {
        faceGroup->init();
    }
}
