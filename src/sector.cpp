#include "sector.hpp"
#include "light.hpp"
#include "scene.hpp"
#include "app.hpp"
#include "sound.hpp"

void Sector::render() {
    //if(!isVisible()) return;
    if(!mOn) return;
    
    for(const auto& frame : mChilds) {
        if(frame->getFrameType() == FrameType::Sector) {
            frame->render();
        }
    }

    App::get()->getScene()->setCurrentSector(this);

    for(const auto& frame : mChilds) {
        if(frame->getFrameType() != FrameType::Sector) {
            frame->render();
        }
    }
}

void Sector::pushLight(std::shared_ptr<Light> light) {
    mSectorLights.push_back(light);
}

void Sector::pushSound(std::shared_ptr<Sound> sound) {
    mSectorSounds.push_back(sound);
}