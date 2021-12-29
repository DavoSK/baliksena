#pragma once
#include <memory>
#include <string>
#include <queue>

#include "mafia/parser_cachebin.hpp"
#include "mafia/parser_scene2bin.hpp"

#include "model.hpp"
#include "renderer.hpp"

class Light;
class Material;
class Model;
class Camera;
class Sector;
class Sound;

class Scene : public Model {
public:
    [[nodiscard]] Camera* getActiveCamera() { return mActiveCamera; }
    void setActiveCamera(Camera* cam) { mActiveCamera = cam; }

    [[nodiscard]] Camera* getDebugCamera() { return mDebugCamera; }

    [[nodiscard]] Sector* getCurrentSector() { return mCurrentSector; }
    void setCurrentSector(Sector* sector) { mCurrentSector = sector; }
    
    [[nodiscard]] Sector* getCameraSector();
    void init();
    void load(const std::string& mission);
    void clear();
    void render();

    void pushAlphaFrame(Frame* frameToPush) { mAlphaPassFrames.push(frameToPush); }
private:
    void createCameras();
    void updateActiveCamera(float deltaTime);
    void initVertexBuffers();
    void getSectorOfPoint(const glm::vec3& pos, Frame* node, std::optional<Sector*>& foundSector);
    std::shared_ptr<Sound> loadSound(const MFFormat::DataFormatScene2BIN::Object& object);
    std::shared_ptr<Light> loadLight(const MFFormat::DataFormatScene2BIN::Object& object);
    std::shared_ptr<Sector> loadSector(const MFFormat::DataFormatScene2BIN::Object& object);
    std::shared_ptr<Model> loadModel(const MFFormat::DataFormatScene2BIN::Object& object);

    std::shared_ptr<Sector> mPrimarySector{ nullptr };
    std::shared_ptr<Sector> mBackdropSector{ nullptr };
    
    Camera* mActiveCamera{ nullptr };
    Camera* mDebugCamera{ nullptr };
    Sector* mCurrentSector{ nullptr };

    Renderer::BufferHandle mVertexBuffer{ 0 };
    Renderer::BufferHandle mIndexBuffer{ 0 };
    std::queue<Frame*> mAlphaPassFrames;
};