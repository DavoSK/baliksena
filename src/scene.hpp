#pragma once
#include <memory>
#include <string>

#include "model.hpp"
#include "renderer.hpp"

class Material;
class Model;
class Camera;
class Sector;

class Scene : public Model {
public:
    Camera* getActiveCamera() { return mActiveCamera.get(); }
    void setActiveCamera(std::shared_ptr<Camera> cam) { mActiveCamera = std::move(cam); }
    
    void load(const std::string& mission);
    void render();
private: 
    std::shared_ptr<Sector> mPrimarySector{ nullptr };
    std::shared_ptr<Sector> mBackdropSector{ nullptr };
    std::shared_ptr<Camera> mActiveCamera{ nullptr };
    Model* mSceneModel{ nullptr };
};