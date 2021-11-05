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
    
    //NOTE: cull only objects in Primary sector
    // const auto& worldBBOX = getWorldBBOX();
    // if ( Renderer::getPass() != Renderer::RenderPass::SKYBOX && 
    //     !Renderer::getFrustum().IsBoxVisible(worldBBOX.first, worldBBOX.second)) {
    //     return;
    // }

    if(mUpdateLights) {
        updateLights();
        mUpdateLights = false;
    }

    Renderer::setLights(mLights);
    Renderer::setModel(getWorldMatrix());

    for (auto& faceGroup : mFaceGroups) {
        faceGroup->render();
    }
}

void Mesh::updateLights() {
    //NOTE: if any dir light will be in sector it will be replaced
    //in following lopp
    mLights.clear();
    for(const auto& light : App::get()->getScene()->getCurrentSector()->getLights()) {
         switch(light->getType()) {
            case LightType::Dir: {
                Renderer::Light rLight {};
                rLight.type = Renderer::LightType::Dir;
                rLight.position = light->getDir();
                rLight.ambient  = light->getAmbient();
                rLight.diffuse  = light->getDiffuse();
                mLights.push_back(rLight);
            } break;

            case LightType::Point: {
                Renderer::Light rLight {};
                rLight.type = Renderer::LightType::Point;
                rLight.position = light->getPos();
                rLight.ambient  = light->getAmbient();
                rLight.diffuse  = light->getDiffuse();
                rLight.rangeFar      = light->getFar();
                rLight.rangeNear    = light->getNear();
                mLights.push_back(rLight);
            } break;

            case LightType::Ambient: {
                Renderer::Light rLight {};
                rLight.type = Renderer::LightType::Ambient;
                rLight.ambient = light->getAmbient();
                mLights.push_back(rLight);
            } break;
            default: {
            } break;
        }
    }

    auto meshPos = glm::vec3(this->getWorldMatrix()[3]);
    auto rankFromLighType = [this, &meshPos](const Renderer::Light& light) -> float {
        switch(light.type) {
            case Renderer::LightType::Ambient:
                return 1;
            case Renderer::LightType::Dir:
                return 2;
            case Renderer::LightType::Point:
                return 3.0f + glm::length2(meshPos - light.position);
        }
    };

    std::sort(mLights.begin(), mLights.end(), [&](Renderer::Light a, Renderer::Light b) {
        return rankFromLighType(a) < rankFromLighType(b);
    });
}