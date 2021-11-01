#include "sector.hpp"
#include "light.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "app.hpp"

void Sector::renderLights() {
     //NOTE: clear dir light for this sector
    Renderer::DirLight defaultDirLight{};
    Renderer::setDirLight(defaultDirLight);

    Renderer::AmbientLight defaultAmbLight{};
    Renderer::setAmbientLight(defaultAmbLight);

    for(const auto& light : mSectorLights) {
         switch(light->getType()) {
            case LightType::Dir: {
                Renderer::DirLight dirLight = {
                    light->getDir(),
                    light->getAmbient(),
                    light->getDiffuse(),
                    light->getSpecular()
                };
                Renderer::setDirLight(dirLight);
            } break;

            case LightType::Ambient: {
                Renderer::AmbientLight ambLight = {
                    light->getAmbient()
                };
                Renderer::setAmbientLight(ambLight);
            } break;

            default: {
            } break;
        }
    }
}

void Sector::render() {
    if(!mOn) return;
    for(const auto& frame : mChilds) {
        if(frame->getFrameType() == FrameType::Sector) {
            frame->render();
        }
    }

    renderLights();
    App::get()->getScene()->setCurrentSector(this);

    for(const auto& frame : mChilds) {
        if(frame->getFrameType() != FrameType::Sector) {
            frame->render();
        }
    }
}

void Sector::pushLight(std::shared_ptr<Light> light) {
    auto it = std::find(mSectorLights.begin(), mSectorLights.end(), light);
    if(it == mSectorLights.end()) {
        mSectorLights.push_back(light);
    }
}