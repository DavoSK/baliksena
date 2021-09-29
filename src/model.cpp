#include "model.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include <memory>

void Model::render() {
    Renderer::bindVertexBuffer(mVertexBuffer);
    Renderer::bindIndexBuffer(mIndexBuffer);
    Frame::render();
}

void Model::init() {
    std::vector<Vertex> modelVertices;
    std::vector<uint32_t> modelIndices;

    for(auto& child : getChilds()) {
        auto* mesh = dynamic_cast<Mesh*>(child.get());
        if(!mesh) continue;

        const auto modelVerticesCount = static_cast<uint32_t>(modelVertices.size());

        for(auto& faceGroup : mesh->getFaceGroups()) {
            std::vector<uint32_t> faceGroupIndices;
            faceGroup->setOffset(modelIndices.size());

            for(auto i : faceGroup->getIndices()) {
                faceGroupIndices.push_back(i + modelVerticesCount);
            }

            modelIndices.insert(modelIndices.end(), faceGroupIndices.begin(), faceGroupIndices.end());
        }

        const auto& meshVertices = mesh->getVertices();
        modelVertices.insert(modelVertices.end(), meshVertices.begin(), meshVertices.end());
    }

    mIndexBuffer = Renderer::createIndexBuffer(modelIndices);
    mVertexBuffer = Renderer::createVertexBuffer(modelVertices);
}