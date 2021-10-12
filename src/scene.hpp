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
    Camera* getActiveCamera() { return mActiveCamera; }
    void setActiveCamera(Camera* cam) { mActiveCamera = cam; }
    
    void load(const std::string& mission);
    void render();
private: 
    std::shared_ptr<Sector> mPrimarySector{ nullptr };
    std::shared_ptr<Sector> mBackdropSector{ nullptr };
    Camera* mActiveCamera{ nullptr };
    Model* mSceneModel{ nullptr };
};