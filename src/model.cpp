#include "model.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "texture.hpp"

#include <functional>
#include <algorithm>
#include <memory>
#include <unordered_map>

void Model::render() {
    /*if (mVertexBuffer.id != 0 && mIndexBuffer.id != 0) {
        Renderer::setVertexBuffer(mVertexBuffer);
        Renderer::setIndexBuffer(mIndexBuffer);
    }*/

    //NOTE: if this model have static geometry
    //render that first
    Renderer::setModel(getMatrix());
    
    /*for (auto& [material, renderHelper] : mRenderHelper) {
        if (material != nullptr)
            material->bind();

        Renderer::bindBuffers();
        Renderer::applyUniforms();
        Renderer::draw(renderHelper.vertexBufferOffset, renderHelper.verticesCount, 1);
    }*/
    
    //NOTE: recursively go trough all childs
    //may potentionaly include dynamic geometry
    Frame::render();
}

void forEachMesh(std::function<void(Mesh*)> callback, Frame* owner) {
    for (const auto& child : owner->getChilds()) {
        auto* frame = child.get();

        //NOTE: if its not mesh it still can hold mesh childs
        auto* mesh = dynamic_cast<Mesh*>(frame);
        if (!mesh) { 
            forEachMesh(callback, frame);
            continue;
        }

        callback(mesh);
        forEachMesh(callback, mesh);
    }
}

void Model::init() {
    struct GeometryBufer {
        std::vector<Renderer::Vertex> vertices;
        std::vector<uint16_t> indices;
        Material* material;
    };

    std::unordered_map<std::string, std::vector<GeometryBufer>> staticBatchingGroup;
    std::vector<Mesh*> dynamicMeshes;

    forEachMesh([this, &staticBatchingGroup, &dynamicMeshes](Mesh* mesh) {
        //NOTE: if mesh is not static we dont care into batching this one
        if (!mesh->isStatic()) {
            dynamicMeshes.push_back(mesh);
            return;
        }

        /*for (const auto& faceGroup : mesh->getFaceGroups()) {
            auto material = faceGroup->getMaterial();
            if(!material) continue;

            auto diffuse = material->getDiffuse();
            if (!diffuse) continue;

            std::vector<Vertex> translatedVertices;
            //NOTE: we are optimizing vertices to contain world position
            //so later we can merge vertices by material and render them as one drawcall
            const auto modelMatrix = mesh->getWorldMatrix();
            for (Vertex vertex : mesh->getVertices()) {
                glm::vec4 translatedVertexPos = modelMatrix * glm::vec4(vertex.p.x, vertex.p.y, vertex.p.z, 1.0f);
                vertex.p = { translatedVertexPos.x, translatedVertexPos.y, translatedVertexPos.z };
                translatedVertices.push_back(vertex);
            }

            staticBatchingGroup[diffuse->getName()].push_back({
                translatedVertices,
                faceGroup->getIndices(),
                material.get()
            });
        }*/
    }, this);

    //NOTE: join geometry by same material
    //save offset in vertex buffer into render helper
    /*for (const auto& [diffuseName, geometry] : staticBatchingGroup) {
        if (geometry.empty()) continue;

        Material* sharedMaterial = geometry[0].material;
        const auto indicesCountBefore = mIndices.size();
        mRenderHelper[sharedMaterial].vertexBufferOffset = indicesCountBefore;
        for (auto& geo : geometry) {
            const auto verticesCount = static_cast<uint32_t>(mVertices.size());
            mVertices.insert(mVertices.end(), geo.vertices.begin(), geo.vertices.end());
            
            for (auto i : geo.indices) {
                mIndices.push_back(i + verticesCount);
            }
        }
        mRenderHelper[sharedMaterial].verticesCount = mIndices.size() - indicesCountBefore;
    }*/

    //NOTE: insert dynamic meshes vertices after static geometry 
    for (Mesh* dynamicMesh : dynamicMeshes) {
        const auto& vertices = dynamicMesh->getVertices();
        const auto currentVerticesCount = static_cast<uint32_t>(mVertices.size());
        mVertices.insert(mVertices.end(), vertices.begin(), vertices.end());

        for (const auto& faceGroup : dynamicMesh->getFaceGroups()) {
            const auto currentIndicesCount = mIndices.size();
            for (auto i : faceGroup->getIndices()) {
                mIndices.push_back(i + currentVerticesCount);
            }
            faceGroup->setOffset(currentIndicesCount);
        }
    }

    mIndexBuffer = Renderer::createIndexBuffer(mIndices);
    mVertexBuffer = Renderer::createVertexBuffer(mVertices);
}