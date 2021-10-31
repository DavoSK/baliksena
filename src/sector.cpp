#include "sector.hpp"
#include "light.hpp"
#include "renderer.hpp"

void Sector::renderLights() {
     //NOTE: clear dir light for this sector
    Renderer::DirLight defaultDirLight{};
    Renderer::setDirLight(defaultDirLight);

    //NOTE: if any dir light will be in sector it will be replaced
    //in following lopp
    std::vector<Renderer::PointLight> pointLights;
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

    Renderer::setPointLights(pointLights);
}

void Sector::render() {
    if(!mOn) return;

    for(const auto& frame : mChilds) {
        if(frame->getFrameType() == FrameType::Sector) {
            frame->render();
        }
    }

    renderLights();
    
    for(const auto& frame : mChilds) {
        if(frame->getFrameType() != FrameType::Sector) {
            frame->render();
        }
    }
}