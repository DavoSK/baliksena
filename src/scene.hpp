#pragma once
#include <memory>

class Camera;
class Scene {
public:
    Scene();
    std::weak_ptr<Camera> getActiveCamera() { return mActiveCamera; }
    void setActiveCamera(std::shared_ptr<Camera> cam) { mActiveCamera = std::move(cam); }
private: 
    std::shared_ptr<Camera> mActiveCamera;
};