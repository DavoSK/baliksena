#include "mesh.hpp"
#include "material.hpp"
#include "scene.hpp"
#include "sector.hpp"
#include "app.hpp"
#include "camera.h"
#include "light.hpp"

/*
    FaceGroup
*/

void FaceGroup::render() const {    
    if(mMaterial != nullptr) {
        if(Renderer::getPass() != Renderer::RenderPass::ALPHA && mMaterial->isTransparent()) {
            if(const auto& mesh = this->mMesh.lock()) {
                App::get()->getScene()->pushAlphaFrame(mesh.get());
            }
            return;
        }
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

    glm::vec3 AABBmin = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 AABBmax = glm::vec3(std::numeric_limits<float>::min());

    for (const auto& vertex : mVertices) {
        AABBmin = glm::min(AABBmin, vertex.p);
        AABBmax = glm::max(AABBmax, vertex.p);
    }

    setBBOX(std::make_pair(AABBmin, AABBmax));
}

void Mesh::render() {
    if(!isVisible()) return;
    
    Frame::render();
    if (mVertices.empty() || !isOn()) return;

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
                rLight.type     = Renderer::LightType::Dir;
                rLight.dir      = light->getDir();
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
                rLight.range    = light->getRange();
                mLights.push_back(rLight);
            } break;

            case LightType::Ambient: {
                Renderer::Light rLight {};
                rLight.type     = Renderer::LightType::Ambient;
                rLight.ambient  = light->getAmbient();
                mLights.push_back(rLight);
            } break;

            case LightType::Spot: {
                Renderer::Light rLight {};
                rLight.type         = Renderer::LightType::Spot;
                rLight.diffuse      = light->getDiffuse();
                rLight.ambient      = light->getAmbient();
                rLight.dir          = light->getDir();
                rLight.position     = light->getPos();
                rLight.cone         = light->getCone();
                rLight.range        = light->getRange();
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
            case Renderer::LightType::Spot:
            case Renderer::LightType::Point:
                return 3.0f + glm::length2(meshPos - light.position);
        }
    };

    //TODO: spot and point lights needs to be sorted
    //by checking bouding volume next to mesh bounding volume
    std::sort(mLights.begin(), mLights.end(), [&](Renderer::Light a, Renderer::Light b) {
        return rankFromLighType(a) < rankFromLighType(b);
    });

    mLights.resize(15);
}