#include "app.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "camera.h"
#include "scene.hpp"

#define SOKOL_TIME_IMPL
#include <sokol/sokol_time.h>

App::App() {
    mInput = std::make_unique<Input>();
    mScene = std::make_unique<Scene>();
}

App::~App() {

}

void App::init() {
    Renderer::init();
    stm_setup();

    auto mainCam = std::make_shared<Camera>();
    mainCam->createProjMatrix(Renderer::getWidth(), Renderer::getHeight());
    mScene->setActiveCamera(mainCam);
}

void App::render() {
    static auto lastTime = stm_now();
    const auto dt = stm_ms(stm_diff(stm_now(), lastTime));
    
    if(auto cam = mScene->getActiveCamera().lock()) {
        cam->setDirDelta(mInput->getMouseDelta());
        cam->setPosDelta(mInput->getMoveDir());
        mInput->clearDeltas();
        cam->update(dt);
        
        RendererCamera renderCamera = { cam->getViewMatrix(), cam->getProjMatrix()};
        Renderer::render(renderCamera);
    }

    lastTime = stm_now();
}

void App::event(const sapp_event* e) {

    //NOTE: update camera proj matrix
    if(e->type == SAPP_EVENTTYPE_RESIZED) {
        if(auto cam = mScene->getActiveCamera().lock()) {
            cam->createProjMatrix(Renderer::getWidth(), Renderer::getHeight());
        }
    }

    mInput->updateFromSokolEvent(e);
}

void App::destroy() {
    delete this;
}
