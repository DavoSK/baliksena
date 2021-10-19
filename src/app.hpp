#pragma once
#include <memory>

struct sapp_event;
class Input;
class Scene;

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
private: 
    std::unique_ptr<Input> mInput = nullptr;
    std::unique_ptr<Scene> mScene = nullptr;
};
