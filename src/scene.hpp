#pragma once
#include <memory>
#include <string>

#include "mafia/parser_cachebin.hpp"
#include "mafia/parser_scene2bin.hpp"

#include "model.hpp"
#include "renderer.hpp"

class Light;
class Material;
class Model;
class Camera;
class Sector;

class Scene : public Model {
public:
    Camera* getActiveCamera() { return mActiveCamera.get(); }
    void setActiveCamera(std::shared_ptr<Camera> cam) { mActiveCamera = cam; }
    void load(const std::string& mission);
    void clear();
    void render();
private:
    void getSectorOfPoint(const glm::vec3& pos, Frame* node, std::optional<Sector*>& foundSector);
    std::shared_ptr<Light> loadLight(const MFFormat::DataFormatScene2BIN::Object& object);
    std::shared_ptr<Sector> loadSector(const MFFormat::DataFormatScene2BIN::Object& object);
    std::shared_ptr<Model> loadModel(const MFFormat::DataFormatScene2BIN::Object& object);
    std::shared_ptr<Sector> mPrimarySector{ nullptr };
    std::shared_ptr<Sector> mBackdropSector{ nullptr };
    std::shared_ptr<Sector> mNearSector{ nullptr };
    std::shared_ptr<Camera> mActiveCamera{ nullptr };
};