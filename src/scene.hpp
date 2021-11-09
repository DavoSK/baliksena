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
    [[nodiscard]] Camera* getActiveCamera() { return mActiveCamera.get(); }
    void setActiveCamera(std::shared_ptr<Camera> cam) { mActiveCamera = cam; }

    [[nodiscard]] Sector* getCurrentSector() { return mCurrentSector; }
    void setCurrentSector(Sector* sector) { mCurrentSector = sector; }
    
    [[nodiscard]]Sector* getCameraSector();
    void load(const std::string& mission);
    void clear();
    void render();
private:
    void initVertexBuffers();
    void getSectorOfPoint(const glm::vec3& pos, Frame* node, std::optional<Sector*>& foundSector);
    std::shared_ptr<Light> loadLight(const MFFormat::DataFormatScene2BIN::Object& object);
    std::shared_ptr<Sector> loadSector(const MFFormat::DataFormatScene2BIN::Object& object);
    std::shared_ptr<Model> loadModel(const MFFormat::DataFormatScene2BIN::Object& object);
    std::shared_ptr<Sector> mPrimarySector{ nullptr };
    std::shared_ptr<Sector> mBackdropSector{ nullptr };
    std::shared_ptr<Camera> mActiveCamera{ nullptr };
    Sector* mCurrentSector{ nullptr };

    //TODO: move
    //std::vector<Renderer::Light> mBatchedLights;
    //std::unordered_map<Material*, RenderHelper> mRenderHelper;
    Renderer::BufferHandle mVertexBuffer{ 0 };
    Renderer::BufferHandle mIndexBuffer{ 0 };
};