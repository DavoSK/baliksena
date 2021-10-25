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

#include "mafia/parser_cachebin.hpp"
#include "mafia/parser_scene2bin.hpp"

#include <functional>
#include <memory>
#include <unordered_map>

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

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

std::vector<std::string> split(std::string const& original, char separator);

void Scene::load(const std::string& missionName) {
    clear();

    Logger::get().info("loading mission {}", missionName);
    setName(missionName);

 
    std::string missionFolder = "MISSIONS\\" + missionName;
    std::string modelsFolder = "MODELS\\";

    // NOTE: load scene.4ds into primary sector
    std::string scenePath = missionFolder + "\\scene.4ds";
    std::shared_ptr<Sector> primarySector = std::make_shared<Sector>();
    
    auto loadedModel = ModelLoader::loadModel(scenePath.c_str());
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
    std::unordered_map<std::string, std::vector<std::shared_ptr<Frame>>> parentingGroup;

    auto getParentNameForObject = [this](MFFormat::DataFormatScene2BIN::Object& obj) -> std::string {
        auto nodeName = std::string(obj.mName.c_str());
        auto parentName = std::string(obj.mParentName.c_str());

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
    if (sceneBinFile.size()) {
        MFFormat::DataFormatScene2BIN sceneBin;
        if (sceneBin.load(sceneBinFile)) {
            for (auto& [objName, obj] : sceneBin.getObjects()) {
                //NOTE: check if node is patch
                if(obj.mIsPatch) {
                    patchObjects.push_back(obj);
                    continue;
                }

                //NOTE: is not patch
                std::shared_ptr<Frame> loadedNode = objectFactory(objName.c_str(), obj);
                loadedNode->setName(obj.mName.c_str());
                loadedNode->setMatrix(getMatrixFromPosScaleRot(obj.mPos, obj.mScale, obj.mRot));
                loadedNode->setOn(!obj.mIsHidden);
                parentingGroup[getParentNameForObject(obj)].push_back(std::move(loadedNode));
            }

            for (auto [parentName, nodes] : parentingGroup) {
                auto parent = this->findNodeMaf(parentName);
                if (parent != nullptr) {
                    for (auto node : nodes) {
                        parent->addChild(std::move(node));
                    }
                } else {
                    for (auto node : nodes) {
                        Logger::get().warn("unable to get parrent: {} for: {}", parentName, node->getName());
                    }
                }
            }

            for(const auto& obj : patchObjects) {
                auto nodeToPatch = this->findNodeMaf(obj.mName);
                if(nodeToPatch) {
                    glm::vec3 origScale;
                    glm::quat origRotation;
                    glm::vec3 origTranslation;
                    glm::vec3 origSkew;
                    glm::vec4 origPerspective;
                    glm::decompose(nodeToPatch->getMatrix(), origScale, origRotation, origTranslation, origSkew, origPerspective);

                    glm::vec3 meshPos {};
                    glm::vec3 meshScale {};
                    glm::quat meshRot {};
                    
                    if( obj.mIsPosPatched ) {
                        meshPos = {obj.mPos.x, obj.mPos.y, obj.mPos.z};
                    } else {
                        meshPos = origTranslation;
                    } 
                    
                    if( obj.mIsScalePatched ) {
                        meshScale = {obj.mScale.x, obj.mScale.y, obj.mScale.z};
                    } else {
                        meshScale = origScale;
                    }

                    if(obj.mIsRotPatched) {
                        meshRot.w = obj.mRot.w;
                        meshRot.x = obj.mRot.x;
                        meshRot.y = obj.mRot.y;
                        meshRot.z = obj.mRot.z;
                    } else {
                        meshRot = origRotation;
                    }

                    const auto translation = glm::translate(glm::mat4(1.f), meshPos);
                    const auto scale = glm::scale(glm::mat4(1.f), meshScale);
                    const auto rot =  glm::mat4(1.f) * glm::toMat4(meshRot);
                    const auto world = translation * rot * scale;

                    nodeToPatch->setMatrix(world);
                    nodeToPatch->setOn(!obj.mIsHidden);      

                    //NOTE: if current parent its not same as patch parent
                    //we need to reparent our node
                    if(!obj.mParentName.empty()) {
                        if(nodeToPatch->getOwner()->getName() != obj.mParentName) {
                            auto patchedNodeParent = this->findNodeMaf(obj.mParentName);
                            if(patchedNodeParent) {
                                nodeToPatch->getOwner()->removeChild(nodeToPatch);
                                patchedNodeParent->addChild(std::move(nodeToPatch));
                            }
                        }
                    }
                } else {
                    Logger::get().warn("unable to find node: {} for patching !", obj.mName);
                }
            }
        }
    }

    //NOTE: load cachus binus
    std::shared_ptr<Sector> nearSector = std::make_shared<Sector>();
    nearSector->setName("Near sector");
    mNearSector = nearSector;
    addChild(nearSector);

    std::string sceneCacheBin = missionFolder + "\\cache.bin";
    MFFormat::DataFormatCacheBIN cacheBin;
    auto cacheBinFile = Vfs::getFile(sceneCacheBin);
    if(cacheBinFile.size() && cacheBin.load(cacheBinFile)) {
        for(const auto& obj : cacheBin.getObjects()) {
            for(const auto& instance : obj.mInstances) {

                auto model = ModelLoader::loadModel(modelsFolder + instance.mModelName);
                if(model != nullptr) {
                    model->setMatrix(getMatrixFromInstance(instance));
                    mNearSector->addChild(std::move(model));
                }
            }
        }
    }

    invalidateTransformRecursively();
    init();
}

void Scene::clear() {
    mPrimarySector = nullptr;
    mNearSector = nullptr;
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

    if(mNearSector != nullptr) {
        mNearSector->render();
    }
    
    //TODO: transparency pass, alpha blending, sorting, etc ...
}