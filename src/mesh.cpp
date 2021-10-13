#include "mesh.hpp"
#include "material.hpp"

/*
    FaceGroup
*/

void FaceGroup::render() const {    
    if(mMaterial != nullptr) {
        mMaterial->bind();
    }

    Renderer::bindBuffers();
    Renderer::applyUniforms();
    Renderer::draw(static_cast<int>(mOffset), static_cast<int>(mIndicesCount), 1);
}

/*
    Mesh
*/

void Mesh::setVertices(std::vector<Vertex> vertices) {
    mVertices = std::move(vertices);

    // NOTE: build AABB bounding box
    glm::vec3 AABBmin, AABBmax;
    AABBmin = mVertices[0].p;
    AABBmax = mVertices[0].p;

    for (const auto& vertex : mVertices) {
        AABBmin = glm::min(AABBmin, vertex.p);
        AABBmax = glm::max(AABBmax, vertex.p);
    }

    setBBOX(std::make_pair(AABBmin, AABBmax));
}

void Mesh::render() {
    Frame::render();

    if (mStatic || mVertices.empty() || !isOn()) return;
    
    const auto& worldBBOX = getWorldBBOX();
     if (!Renderer::getFrustum().IsBoxVisible(worldBBOX.first, worldBBOX.second)) {
         return;
     }

    Renderer::setModel(getWorldMatrix());

    for (auto& faceGroup : mFaceGroups) {
        faceGroup->render();
    }
}