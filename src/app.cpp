#include "app.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "camera.h"
#include "scene.hpp"
#include "texture.hpp"
#include "stats.hpp"
#include "vfs.hpp"

Stats gStats{};

App::App() {
    mInput = std::make_shared<Input>();
    mScene = std::make_shared<Scene>();
}

void App::init() {
    Vfs::init("C:\\Mafia\\");
    Renderer::init();

    //NOTE: create main camera
    auto mainCam = std::make_shared<Camera>();
    mainCam->createProjMatrix(Renderer::getWidth(), Renderer::getHeight());
    mScene->setActiveCamera(mainCam);
}

void App::render() {
    Renderer::begin();
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
    Vfs::destroy();
}
