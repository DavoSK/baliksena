#include "app.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "camera.h"
#include "scene.hpp"
#include "texture.hpp"
#include "stats.hpp"

Stats gStats{};

App::App() {
    mInput = new Input();
    mScene = new Scene();
}

App::~App() {
    //NOTE: dealocate main camera from scene
    auto* mainCam = mScene->getActiveCamera();
    if(mainCam != nullptr) {
        delete mainCam;
    } 

    delete mInput;
    delete mScene;
}

void App::init() {
    Renderer::init();
  
    auto* mainCam = new Camera();
    mainCam->createProjMatrix(Renderer::getWidth(), Renderer::getHeight());
    mScene->setActiveCamera(mainCam);
    //mScene->load("FREERIDE");
}

void App::render() { 
    const auto deltaTime = 16.0f;

    //NOTE: update camera & render
    if(auto* cam = mScene->getActiveCamera()) {
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
    mScene->render();
    Renderer::end();
    Renderer::commit();
}

void App::event(const sapp_event* e) {
    //NOTE: update camera proj matrix
    if(e->type == sapp_event_type::SAPP_EVENTTYPE_RESIZED) {
        if(auto* cam = mScene->getActiveCamera()) {
            cam->createProjMatrix(Renderer::getWidth(), Renderer::getHeight());
        }
    }

    Renderer::guiHandleSokolInput(e);
    mInput->updateFromSokolEvent(e);
}

void App::destroy() {
    delete this;
    Texture::clearCache();
    Renderer::destroy();
}
