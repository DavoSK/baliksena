#pragma once
#include <memory>

struct sapp_event;
class Input;
class Scene;
class Model;

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
private: 
    std::unique_ptr<Input> mInput;
    std::unique_ptr<Scene> mScene;
    std::shared_ptr<Model> testModel;
};
