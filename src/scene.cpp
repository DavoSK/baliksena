#include "scene.hpp"
#include "camera.h"
#include "model.hpp"
#include "mesh.hpp"
#include "model_loader.hpp"
#include "texture.hpp"
#include "material.hpp"
#include "logger.hpp"
#include "sector.hpp"

#include "mafia/parser_cachebin.hpp"
#include "mafia/parser_scene2bin.hpp"

#include <functional>
#include <memory>
#include <unordered_map>

#include <glm/gtx/quaternion.hpp>

[[nodiscard]] glm::mat4 getMatrixFromPosScaleRot(const MFMath::Vec3& mpos, const MFMath::Vec3& mscale, const MFMath::Quat& mrot) {
    glm::vec3 meshPos = {mpos.x, mpos.y, mpos.z};
    glm::vec3 meshScale = {mscale.x, mscale.y, mscale.z};
    glm::quat meshRot;

    meshRot.w = mrot.w;
    meshRot.x = mrot.x;
    meshRot.y = mrot.y;
    meshRot.z = mrot.z;

    const auto translation = glm::translate(glm::mat4(1.f), meshPos);
    const auto scale = glm::scale(glm::mat4(1.f), meshScale);
    const auto rot = glm::mat4(1.f) * glm::toMat4(meshRot);
    const auto world = translation * rot * scale;
    return world;
}

[[nodiscard]] glm::mat4 getMatrixFromInstance(const MFFormat::DataFormatCacheBIN::Instance& instance) {
    return getMatrixFromPosScaleRot(instance.mPos, instance.mScale, instance.mRot);
}

void getSectorOfPoint(const glm::vec3& pos, Frame* node, std::optional<Sector*>& foundSector) {    
    if (!node) return;

    const auto isPointInsideAABB = [](const glm::vec3& point, const glm::vec3& min, const glm::vec3& max) -> bool {
        return (point.x >= min.x && point.x <= max.x) && (point.y >= min.y && point.y <= max.y) && (point.z >= min.z && point.z <= max.z);
    };

    if (node->getType() == FrameType::SECTOR) {
        const auto& sectorBBox = node->getBBOX();
        const auto& mat = node->getWorldMatrix();
        const auto min = glm::translate(mat, sectorBBox.first);
        const auto max = glm::translate(mat, sectorBBox.second);

        if (isPointInsideAABB(pos, min[3], max[3])) {
            foundSector = reinterpret_cast<Sector*>(node);
        } else return;
    }

    for (const auto& node : node->getChilds()) {
        getSectorOfPoint(pos, node.get(), foundSector);
    }
};

void Scene::load(const std::string& missionName) {
    Logger::get().info("loading mission {}", missionName);
    std::string gameFolder = "C:/Mafia";
    std::string missionFolder = gameFolder + "/MISSIONS/" + missionName + "/";
    std::string modelsFolder = gameFolder + "/MODELS/";

    // NOTE: load scene.4ds into primary sector
    std::string scenePath = missionFolder + "/scene.4ds";
    std::shared_ptr<Sector> primarySector = std::make_shared<Sector>();
    primarySector->addChild( ModelLoader::loadModel(scenePath.c_str()));
    primarySector->setName("Primary sector");
    
    mPrimarySector = primarySector;
    addChild(primarySector);

    // NOTE: backdrop sector used for skyboxes and all objects
    // rendered relative to camera
    std::shared_ptr<Sector> backdropSector = std::make_shared<Sector>();
    backdropSector->setName("Backdrop sector");

    mBackdropSector = backdropSector;
    addChild(backdropSector);

    auto loadSceneModel = [modelsFolder](const std::string& objpath, MFFormat::DataFormatScene2BIN::Object& object) -> std::shared_ptr<Frame> {
        auto modelPath = modelsFolder + std::string(object.mModelName.c_str());
        return ModelLoader::loadModel(modelPath.c_str());
    };

    auto loadSector = [&](const std::string& objPath, MFFormat::DataFormatScene2BIN::Object& object) -> std::shared_ptr<Sector> {
        std::shared_ptr<Sector> parent = std::make_shared<Sector>();
        parent->setName( object.mName.c_str());
        return parent;
    };

    auto objectFactory = [&](const std::string& objName, MFFormat::DataFormatScene2BIN::Object& obj) -> std::shared_ptr<Frame> {
        switch (obj.mType) {
            // case MFFormat::DataFormatScene2BIN::ObjectType::OBJECT_TYPE_LIGHT: {
            //     return loadSceneLight(objName, obj);
            // } break;

            case MFFormat::DataFormatScene2BIN::ObjectType::OBJECT_TYPE_MODEL: {
                return loadSceneModel(objName, obj);
            } break;

            case MFFormat::DataFormatScene2BIN::ObjectType::OBJECT_TYPE_SECTOR: {
                return loadSector(objName, obj);
            } break;
        }

        auto dummyFrame = std::make_shared<Frame>();
        dummyFrame->setName(objName);
        return dummyFrame;
    };

    // NOTE: load scene2 bin
    std::string sceneBinPath = missionFolder + "/scene2.bin";
    std::ifstream sceneBinFile(sceneBinPath.c_str(), std::ifstream::binary);

    if (sceneBinFile.good()) {
        MFFormat::DataFormatScene2BIN sceneBin;
        if (sceneBin.load(sceneBinFile)) {
            for (auto& [objName, obj] : sceneBin.getObjects()) {
                std::shared_ptr<Frame> loadedNode = objectFactory(objName.c_str(), obj);
                loadedNode->setName(obj.mName.c_str());
                loadedNode->setMatrix(getMatrixFromPosScaleRot(obj.mPos, obj.mScale, obj.mRot));
                loadedNode->setOn(!obj.mIsHidden);

                auto nodeName = std::string(obj.mName.c_str());
                auto parentName = std::string(obj.mParentName.c_str());

                if (parentName.length() == 0) {
                    glm::vec3 worldPosOfSector = { obj.mPos2.x, obj.mPos2.y, obj.mPos2.z };
                    if (worldPosOfSector.x == 0 && 
                        worldPosOfSector.y == 0 && 
                        worldPosOfSector.z == 0) {
                        primarySector->addChild(loadedNode);
                        continue;
                    }

                    std::optional<Sector*> foundSector;
                    getSectorOfPoint(worldPosOfSector, this, foundSector);
                    
                    if (foundSector.has_value()) {
                        foundSector.value()->addChild(loadedNode);
                    }
                } else {
                    auto foundNode = this->findNodeMaf(parentName);
                    if (foundNode) {
                        foundNode->addChild(loadedNode);
                    }
                }
            }
        }
    }

    init();
}

void Scene::render() {
    Model::render();
}