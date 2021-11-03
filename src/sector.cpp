#include "sector.hpp"
#include "light.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "app.hpp"

void Sector::renderLights() {
    //NOTE: clear lights for this sector
    std::vector<Renderer::Light> emptyLights;
    Renderer::setLights(emptyLights);
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
    //auto it = std::find(mSectorLights.begin(), mSectorLights.end(), light);
    //if(it == mSectorLights.end()) {
        mSectorLights.push_back(light);
    //}
}