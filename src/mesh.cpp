#include "mesh.hpp"
#include "material.hpp"
#include "scene.hpp"
#include "sector.hpp"
#include "app.hpp"
#include "light.hpp"

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

void Mesh::setVertices(std::vector<Renderer::Vertex> vertices) {
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

    //NOTE: cull only objects in Primary sector
    // if (!Renderer::getFrustum().IsBoxVisible(worldBBOX.first, worldBBOX.second) && 
    //     Renderer::getPass() != Renderer::RenderPass::SKYBOX) {
    //     return;
    // }

    if(mUpdateLights) {
        updateLights();
        mUpdateLights = false;
    }

    Renderer::setPointLights(mLights);
    Renderer::setModel(getWorldMatrix());

    for (auto& faceGroup : mFaceGroups) {
        faceGroup->render();
    }
}

void Mesh::updateLights() {
    //NOTE: if any dir light will be in sector it will be replaced
    //in following lopp
    std::vector<Renderer::PointLight> pointLights;
    for(const auto& light : App::get()->getScene()->getCurrentSector()->getLights()) {
         switch(light->getType()) {
            case LightType::Point: {
                Renderer::PointLight pointLight = {
                    light->getPos(),
                    light->getAmbient(),
                    light->getDiffuse(),
                    light->getSpecular(),
                    light->getRange()
                };
                pointLights.push_back(pointLight);
            } break;

            default: {
            } break;
        }
    }

    //NOTE: take nearest 8 point lights
    mLights.clear();
    float minDist = std::numeric_limits<float>::max();

    for(auto pointLight : pointLights) {
        auto meshPos = glm::vec3(this->getWorldMatrix()[3]);
        auto dist = glm::length2(meshPos - pointLight.position);
        if(dist < minDist) {
            minDist = dist;
            mLights.push_back(pointLight);
        }
    }

    mLights.resize(30);
}