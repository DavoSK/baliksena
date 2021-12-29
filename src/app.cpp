#include "app.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "camera.hpp"
#include "scene.hpp"
#include "texture.hpp"
#include "stats.hpp"
#include "vfs.hpp"
#include "audio.hpp"

Stats gStats{};

App::App() {
    mInput = std::make_shared<Input>();
    mScene = std::make_shared<Scene>();
    mAudio = std::make_shared<Audio>();
}

void App::init() {
    //NOTE: debug different mount points
    //only for now :)
    #ifdef __linux__
    Vfs::init("/home/david/dev/Mafia/");
    #else 
    Vfs::init("C:\\Mafia\\");
    #endif

    Renderer::init();

    mScene->init();
    mAudio->init();
}

void App::render() {
    mAudio->update();
    
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
