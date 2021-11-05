#include "scene.hpp"
#include "camera.h"
#include "model.hpp"
#include "mesh.hpp"
#include "model_loader.hpp"
#include "texture.hpp"
#include "material.hpp"
#include "logger.hpp"
#include "sector.hpp"
#include "vfs.hpp"
#include "input.hpp"
#include "app.hpp"
#include "light.hpp"
#include "mafia/utils.hpp"

#include <functional>
#include <memory>
#include <unordered_map>

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

Sector* Scene::getCameraSector() {
    if(mActiveCamera != nullptr) {
        std::optional<Sector*> foundSector;
        getSectorOfPoint(mActiveCamera->Position, this, foundSector);
        return foundSector.value_or(mPrimarySector.get());
    }
    return nullptr;
}

void Scene::getSectorOfPoint(const glm::vec3& pos, Frame* node, std::optional<Sector*>& foundSector) {    
    if (!node) return;

    const auto isPointInsideAABB = [](const glm::vec3& point, const glm::vec3& min, const glm::vec3& max) -> bool {
        return (point.x >= min.x && point.x <= max.x) && (point.y >= min.y && point.y <= max.y) && (point.z >= min.z && point.z <= max.z);
    };

    if (node->getFrameType() == FrameType::Sector) {
        const auto& sectorBBox = node->getBBOX();
        const auto& mat = node->getWorldMatrix();
        const auto min = glm::translate(mat, sectorBBox.first);
        const auto max = glm::translate(mat, sectorBBox.second);

        if (isPointInsideAABB(pos, min[3], max[3])) {
            foundSector = reinterpret_cast<Sector*>(node);
        }
    }

    for (const auto& node : node->getChilds()) {
        getSectorOfPoint(pos, node.get(), foundSector);
    }
};

std::unordered_map<std::string, std::vector<std::shared_ptr<Light>>> gLightsParenting;

std::shared_ptr<Light> Scene::loadLight(const MFFormat::DataFormatScene2BIN::Object& object) {
    auto light = std::make_shared<Light>();
    //NOTE: skip this light ( dunno why )
    if (!(object.mLightFlags & (1 << 0))) 
        return light;

    switch (object.mLightType) {
        case MFFormat::DataFormatScene2BIN::LightType::LIGHT_TYPE_DIRECTIONAL: {
            auto color = glm::vec3(object.mLightColour.x, object.mLightColour.y, object.mLightColour.z) * object.mLightPower;
            light->setType(LightType::Dir);
            light->setDiffuse(color);

            glm::quat meshRot {};
            meshRot.w = object.mRot.w;
            meshRot.x = object.mRot.x;
            meshRot.y = object.mRot.y;
            meshRot.z = object.mRot.z;

            auto mat4 = glm::toMat4(meshRot);
            auto dir = glm::vec3(mat4[2]);
            light->setDir(dir);
        } break;
        
        // case MFFormat::DataFormatScene2BIN::LightType::LIGHT_TYPE_AMBIENT: {
        //     auto color = glm::vec3(object.mLightColour.x, object.mLightColour.y, object.mLightColour.z) * object.mLightPower;
        //     light->setType(LightType::Ambient);
        //     light->setAmbient(color);
        // } break;
        
        case MFFormat::DataFormatScene2BIN::LightType::LIGHT_TYPE_SPOT: {
            // auto color = glm::vec3(object.mLightColour.x, object.mLightColour.y, object.mLightColour.z) * object.mLightPower;
            // light->setType(LightType::Ambient);
            // light->setAmbient(glm::normalize(color));
            //Logger::get().info("info spot light {}", object.mName);
            return light;
        } break;

        case MFFormat::DataFormatScene2BIN::LightType::LIGHT_TYPE_POINT: {
            auto color = glm::vec3(object.mLightColour.x, object.mLightColour.y, object.mLightColour.z) * object.mLightPower;
            light->setType(LightType::Point);
            light->setPos({object.mPos2.x, object.mPos2.y, object.mPos2.z});
            light->setDiffuse(color);
            light->setFar(object.mLightFar);
            light->setNear(object.mLightNear);
        } break;

        default: {
            return light;
        } break;
    }

    for(const auto& splitedString : object.mLightSectors) {
        gLightsParenting[splitedString].push_back(light);        
    }

    return light;
}

std::shared_ptr<Sector> Scene::loadSector(const MFFormat::DataFormatScene2BIN::Object& object) {
    return std::make_shared<Sector>();
}

std::shared_ptr<Model> Scene::loadModel(const MFFormat::DataFormatScene2BIN::Object& object) {
    return ModelLoader::loadModel("MODELS\\" + object.mModelName, object.mName);
}

void Scene::load(const std::string& missionName) {
    clear();
    gLightsParenting.clear();

    Logger::get().info("loading mission {}", missionName);
    setName(missionName);

 
    std::string missionFolder = "MISSIONS\\" + missionName;
    std::string modelsFolder = "MODELS\\";

    // NOTE: load scene.4ds into primary sector
    std::string scenePath = missionFolder + "\\scene.4ds";
    std::shared_ptr<Sector> primarySector = std::make_shared<Sector>();
    
    auto loadedModel = ModelLoader::loadModel(scenePath.c_str(), "scene");
    for (auto childNode : loadedModel->getChilds()) {
        primarySector->addChild(childNode);
    }

    primarySector->setName("Primary sector");
    
    mPrimarySector = primarySector;
    addChild(primarySector);

    // NOTE: backdrop sector used for skyboxes and all objects
    // rendered relative to camera
    std::shared_ptr<Sector> backdropSector = std::make_shared<Sector>();
    backdropSector->setName("Backdrop sector");

    mBackdropSector = backdropSector;
    addChild(backdropSector);

    auto objectFactory = [&](MFFormat::DataFormatScene2BIN::Object& obj) -> std::shared_ptr<Frame> {
        switch (obj.mType) {
            case MFFormat::DataFormatScene2BIN::ObjectType::OBJECT_TYPE_LIGHT: {
                return loadLight(obj);
            } break;

            case MFFormat::DataFormatScene2BIN::ObjectType::OBJECT_TYPE_MODEL: {
                return loadModel(obj);
            } break;

            case MFFormat::DataFormatScene2BIN::ObjectType::OBJECT_TYPE_SECTOR: {
                return loadSector(obj);
            } break;
        }

        return std::make_shared<Frame>();
    };

    // NOTE: load scene2 bin
    std::unordered_map<std::string, std::vector<std::shared_ptr<Frame>>> parentingGroup;
 
    auto getParentNameForObject = [this](MFFormat::DataFormatScene2BIN::Object& obj) -> std::string {
        auto nodeName = obj.mName;
        auto parentName = obj.mParentName;

        if (parentName.length() == 0) {
            glm::vec3 worldPosOfSector = { obj.mPos2.x, obj.mPos2.y, obj.mPos2.z };
            if (worldPosOfSector.x == 0 &&
                worldPosOfSector.y == 0 &&
                worldPosOfSector.z == 0) {
                return "Primary sector";
            }

            std::optional<Sector*> foundSector;
            getSectorOfPoint(worldPosOfSector, this, foundSector);

            if (foundSector.has_value()) {
                return foundSector.value()->getName();
            }

            return "Primary sector";
        }
        
        return parentName;
    };

    std::vector<MFFormat::DataFormatScene2BIN::Object> patchObjects;

    std::string sceneBinPath = missionFolder + "\\scene2.bin";
    auto sceneBinFile = Vfs::getFile(sceneBinPath);
    if (sceneBinFile.has_value()) {
        MFFormat::DataFormatScene2BIN sceneBin;
        if (sceneBin.load(sceneBinFile.value())) {
            for (auto& [objName, obj] : sceneBin.getObjects()) {
                //NOTE: check if node is patch
                if(obj.mIsPatch) {
                    patchObjects.push_back(obj);
                    continue;
                }

                //NOTE: is not patch
                std::shared_ptr<Frame> loadedNode = objectFactory(obj);
                loadedNode->setName(obj.mName);
                loadedNode->setPos({obj.mPos.x, obj.mPos.y, obj.mPos.z});
                loadedNode->setScale({obj.mScale.x, obj.mScale.y, obj.mScale.z});

                glm::quat meshRot {};
                meshRot.w = obj.mRot.w;
                meshRot.x = obj.mRot.x;
                meshRot.y = obj.mRot.y;
                meshRot.z = obj.mRot.z;
                loadedNode->setRot(meshRot);
                loadedNode->setOn(!obj.mIsHidden);
                parentingGroup[getParentNameForObject(obj)].push_back(std::move(loadedNode));
            }

            //NOTE: multiple times cuz nodes are not sorted by dependecies of parents
            for(size_t i = 0; i < 5; i++) {
                for (auto& [parentName, nodes] : parentingGroup) {
                    auto parent = this->findFrame(parentName);
                    if (parent != nullptr) {
                        for (auto node : nodes) {
                            parent->addChild(std::move(node));
                        }
                        nodes.clear();
                    } else {
                        if(i == 4) {
                            for (auto node : nodes) {
                                Logger::get().error("unable to get parrent: {} for: {}", parentName, node->getName());
                            }
                        }
                    }
                }
            }

            for(const auto& obj : patchObjects) {
                auto nodeToPatch = this->findFrame(obj.mName);
                if(nodeToPatch) {
                    if(obj.mIsPosPatched) {
                        nodeToPatch->setPos({obj.mPos.x, obj.mPos.y, obj.mPos.z});
                    }
                    
                    if(obj.mIsScalePatched) {
                        nodeToPatch->setScale({obj.mScale.x, obj.mScale.y, obj.mScale.z});
                    }

                    if(obj.mIsRotPatched) {
                        glm::quat meshRot {};
                        meshRot.w = obj.mRot.w;
                        meshRot.x = obj.mRot.x;
                        meshRot.y = obj.mRot.y;
                        meshRot.z = obj.mRot.z;
                        nodeToPatch->setRot(meshRot);
                    }

                    nodeToPatch->setOn(!obj.mIsHidden);      

                    //NOTE: look if we need to reparent
                    if(obj.mIsParentPatched) {
                        auto patchedNodeParent = this->findFrame(obj.mParentName);
                        if(patchedNodeParent) {
                            nodeToPatch->getOwner()->removeChild(nodeToPatch);
                            patchedNodeParent->addChild(std::move(nodeToPatch));
                        }
                    }
                } else {
                    Logger::get().warn("[PATCH] unable to find node: {} for !", obj.mName);
                }
            }

            for(const auto& [lightSector, lights] : gLightsParenting) {
                auto foundSector = findFrame(lightSector);
                if (foundSector != nullptr && foundSector->getFrameType() == FrameType::Sector) {
                    for(auto lightToPush : lights) {
                        (std::dynamic_pointer_cast<Sector>(foundSector))->pushLight(lightToPush);
                    }
                }
            }

            invalidateTransformRecursively();
        }
    }

    //NOTE: load cachus binus
    std::string sceneCacheBin = missionFolder + "\\cache.bin";
    MFFormat::DataFormatCacheBIN cacheBinFormat;
    auto cacheBinFile = Vfs::getFile(sceneCacheBin);
    if(cacheBinFile.has_value() && cacheBinFormat.load(cacheBinFile.value())) {
        for(const auto& obj : cacheBinFormat.getObjects()) {
            for(const auto& instance : obj.mInstances) {
                auto model = ModelLoader::loadModel(modelsFolder + instance.mModelName, obj.mObjectName);
                if(model != nullptr) {
                    model->setPos({instance.mPos.x, instance.mPos.y, instance.mPos.z});
                    model->setScale({instance.mScale.x, instance.mScale.y, instance.mScale.z});
                    glm::quat meshRot {};
                    meshRot.w = instance.mRot.w;
                    meshRot.x = instance.mRot.x;
                    meshRot.y = instance.mRot.y;
                    meshRot.z = instance.mRot.z;
                    model->setRot(meshRot);
                    mPrimarySector->addChild(std::move(model));
                }
            }
        }
    }

    invalidateTransformRecursively();
    init();
}

void Scene::clear() {
    mPrimarySector = nullptr;
    mBackdropSector = nullptr;
    removeChilds();
    
    mIndices.clear();
    mVertices.clear();

    if(mIndexBuffer.id != Renderer::InvalidHandle && 
       mVertexBuffer.id != Renderer::InvalidHandle) {
        Renderer::destroyBuffer(mIndexBuffer);
        Renderer::destroyBuffer(mVertexBuffer);
    }
}

void Scene::render() {
    if(!isOn()) return;

    const auto deltaTime = 16.0f;

    //NOTE: update camera & render
    if(auto* cam = getActiveCamera()) {
        auto input = App::get()->getInput();
        if(input->isMouseLocked()) {
            cam->setDirDelta(input->getMouseDelta());
            cam->setPosDelta(input->getMoveDir());
            cam->update(deltaTime);
        }

        input->clearDeltas();
        Renderer::setViewMatrix(cam->getViewMatrix());
        Renderer::setViewPos(cam->Position);
    }

    //NOTE: bind buffers
    if (mVertexBuffer.id != Renderer::InvalidHandle && 
        mIndexBuffer.id != Renderer::InvalidHandle) {
        Renderer::setVertexBuffer(mVertexBuffer);
        Renderer::setIndexBuffer(mIndexBuffer);
    }

    //NOTE: skybox pass -> render Backdrop sector first
    Renderer::setPass(Renderer::RenderPass::SKYBOX);
    Renderer::setProjMatrix(mActiveCamera->getSkyboxProjMatrix());
    
    if(mBackdropSector != nullptr) {
        mBackdropSector->render();
    }

    //NOTE: normal pass -> render normal objects
    Renderer::setPass(Renderer::RenderPass::NORMAL);
    Renderer::setProjMatrix(mActiveCamera->getProjMatrix());

    if(mPrimarySector != nullptr) {
        mPrimarySector->render();
    }
    //TODO: transparency pass, alpha blending, sorting, etc ...
}