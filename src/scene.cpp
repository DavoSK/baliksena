#include "scene.hpp"
#include "camera.h"
#include "model.hpp"
#include "mesh.hpp"
#include "model_loader.hpp"
#include "texture.hpp"
#include "material.hpp"
#include "logger.hpp"
#include "mafia/parser_cachebin.hpp"

#include <glm/gtx/quaternion.hpp>
#include <functional>
#include <memory>
#include <unordered_map>

[[nodiscard]] glm::mat4 getMatrixFromInstance(const MFFormat::DataFormatCacheBIN::Instance& instance) {
    glm::vec3 meshPos = { instance.mPos.x, instance.mPos.y, instance.mPos.z };
    glm::vec3 meshScale = { instance.mScale.x, instance.mScale.y, instance.mScale.z };
    glm::quat meshRot;

    meshRot.w = instance.mRot.w;
    meshRot.x = instance.mRot.x;
    meshRot.y = instance.mRot.y;
    meshRot.z = instance.mRot.z;

    const auto translation = glm::translate(glm::mat4(1.f), meshPos);
    const auto scale = glm::scale(glm::mat4(1.f), meshScale);
    const auto rot = glm::mat4(1.f) * glm::toMat4(meshRot);
    const auto world = translation * rot * scale;
    return world;
}

void Scene::load(const std::string& missionName) {
    Logger::get().info("loading mission {}", missionName);
    
    std::string missionDir = "C:\\Mafia\\MISSIONS\\";

    //NOTE: load scene.4ds
    std::string sceneModelPath = missionDir + missionName + "\\scene.4ds"; 
    addChild(std::move(ModelLoader::loadModel(sceneModelPath)));

    // //NOTE: load cachus binus
    std::string sceneCacheBin = missionDir + missionName + "\\cache.bin";
    std::string modelPath = "C:\\Mafia\\MODELS\\";

    MFFormat::DataFormatCacheBIN cacheBin;
    std::ifstream cacheBinFile(sceneCacheBin, std::ifstream::binary);
    if(cacheBinFile.good() && cacheBin.load(cacheBinFile)) {
        for(const auto& obj : cacheBin.getObjects()) {
            for(const auto& instance : obj.mInstances) {

                auto model = ModelLoader::loadModel(modelPath + instance.mModelName);
                if(model != nullptr) {
                    model->setMatrix(getMatrixFromInstance(instance));
                    addChild(std::move(model));
                }
            }
        }
    }

    init();
}

void Scene::render() {
    //mSceneModel->render();
    //Frame::render();
    Model::render();
}