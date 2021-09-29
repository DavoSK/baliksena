#include "app.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "camera.h"
#include "scene.hpp"
#include "texture.hpp"

#define SOKOL_TIME_IMPL
#include <sokol/sokol_time.h>
#include "model.hpp"
#include "model_loader.hpp"

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

    testModel = ModelLoader::loadModel("C:\\Mafia\\MISSIONS\\EXTREME\\scene.4ds");
}

void App::render() {
    static uint64_t lastTime = stm_now();    
    const auto deltaTime = 16.0f;//static_cast<float>(stm_ms(stm_diff(stm_now(), lastTime)));

    //NOTE: update camera & render
    if(auto cam = mScene->getActiveCamera().lock()) {
        if(mInput->isMouseLocked()) {
            cam->setDirDelta(mInput->getMouseDelta());
            cam->setPosDelta(mInput->getMoveDir());
            cam->update(deltaTime);
        }

        mInput->clearDeltas();
        Renderer::setProjMatrix(cam->getProjMatrix());
        Renderer::setViewMatrix(cam->getViewMatrix());
    }

    Renderer::begin(RenderPass::NORMAL);
    testModel->render();
    Renderer::end();
    Renderer::commit();
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

int gFrameCount;

void App::destroy() {
    delete this;
    Texture::clearCache();
    Renderer::destroy();
}
