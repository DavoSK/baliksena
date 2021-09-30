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
    std::weak_ptr<Camera> getActiveCamera() { return mActiveCamera; }
    void setActiveCamera(std::shared_ptr<Camera> cam) { mActiveCamera = std::move(cam); }
    void load(const std::string& mission);
    void render() override;
private: 
    std::shared_ptr<Camera> mActiveCamera;
    std::shared_ptr<Model> mSceneModel;
};