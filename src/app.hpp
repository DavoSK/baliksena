#pragma once

struct sapp_event;
class Input;
class Scene;

class App {
public: 
    App();
    ~App();
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

    Scene* getScene() { return mScene; }
private: 
    Input* mInput = nullptr;
    Scene* mScene = nullptr;
};
