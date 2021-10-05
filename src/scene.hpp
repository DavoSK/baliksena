#pragma once
#include <memory>
#include <string>

#include "model.hpp"
#include "renderer.hpp"

class Material;
class Model;
class Camera;
class Scene : public Model {
public:
    Camera* getActiveCamera() { return mActiveCamera; }
    void setActiveCamera(Camera* cam) { mActiveCamera = cam; }
    
    void load(const std::string& mission);
    void render();
private: 
    Camera* mActiveCamera = nullptr;
    Model* mSceneModel = nullptr;
};