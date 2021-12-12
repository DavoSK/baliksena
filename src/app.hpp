#pragma once
#include <memory>

struct sapp_event;
class Input;
class Scene;
class Audio;

class App {
public: 
    App();
    void init();
    void render();
    void event(const sapp_event* e);
    void destroy();

    static App* get() {
        static App* app = nullptr;
        if(!app)
            app = new App();
        return app;
    }

    Scene* getScene() { return mScene.get(); }
    Input* getInput() { return mInput.get(); }
    Audio* getAudio() { return mAudio.get(); }
private: 
    std::shared_ptr<Audio> mAudio { nullptr };
    std::shared_ptr<Input> mInput { nullptr };
    std::shared_ptr<Scene> mScene { nullptr };
};
