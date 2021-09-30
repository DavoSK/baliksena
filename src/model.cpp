#include "model.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "texture.hpp"

#include <functional>
#include <memory>
#include <unordered_map>

void Model::render() {
    Renderer::setVertexBuffer(mVertexBuffer);
    Renderer::setIndexBuffer(mIndexBuffer);
    Renderer::setModel(glm::mat4(1.0f));

    for (auto& [material, renderHelper] : mRenderHelper) {
        if (material != nullptr)
            material->bind();

        Renderer::bindBuffers();
        Renderer::applyUniforms();
        Renderer::draw(renderHelper.vertexBufferOffset, renderHelper.verticesCount, 1);
    }
    //Frame::render();
}

//bool areMaterialsEqual(Material* mat1, Material* mat2) {
//    if (mat1->hasTexture(TextureSlots::DIFFUSE) && mat2->hasTexture(TextureSlots::DIFFUSE)) {
//        auto tex1 = mat1->getTexture(TextureSlots::DIFFUSE).lock();
//        auto tex2 = mat2->getTexture(TextureSlots::DIFFUSE).lock();
//        if (tex1 && tex1) {
//            return tex1->getName() == tex2->getName();
//        }
//    }
//    return false;
//}

void forEachMesh(std::function<void(Mesh*)> callback, Frame* owner) {
    for (const auto& child : owner->getChilds()) {
        auto* mesh = dynamic_cast<Mesh*>(child.get());
        if (!mesh) continue;

        callback(mesh);
        forEachMesh(callback, mesh);
    }
}


void Model::init() {
    invalidateTransformRecursively();

    struct GeometryBufer {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
        Material* material;
    };

    std::unordered_map<std::string, std::vector<GeometryBufer>> groupByMaterial;

    forEachMesh([this, &groupByMaterial](Mesh* mesh) {
        for (const auto& faceGroup : mesh->getFaceGroups()) {
            auto material = faceGroup->getMaterial();
            auto diffuse = material->getTexture(TextureSlots::DIFFUSE).lock();

            if (diffuse != nullptr) {

                std::vector<Vertex> translatedVertices;

                //NOTE: we are optimizing vertices to contain world position
                //so later we can merge vertices by material and render them as one drawcall
                const auto modelMatrix = mesh->getWorldMatrix();
                for (Vertex vertex : mesh->getVertices()) {
                    glm::vec4 translatedVertexPos = modelMatrix * glm::vec4(vertex.p.x, vertex.p.y, vertex.p.z, 1.0f);
                    vertex.p = { translatedVertexPos.x, translatedVertexPos.y, translatedVertexPos.z };
                    translatedVertices.push_back(vertex);
                }

                groupByMaterial[diffuse->getName()].push_back({
                    translatedVertices,
                    faceGroup->getIndices(),
                    material.get()
                });
            }
        }

    }, this);
  
    //NOTE: join geometry by same material
    //save offset in vertex buffer into render helper
    for (const auto& [diffuseName, geometry] : groupByMaterial) {
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
    }

    mIndexBuffer = Renderer::createIndexBuffer(mIndices);
    mVertexBuffer = Renderer::createVertexBuffer(mVertices);
}