#include "model_loader.hpp"
#include "mafia/parser_4ds.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "renderer.hpp"
#include "mesh.hpp"
#include "billboard.hpp"
#include "material.hpp"
#include "texture.hpp"
#include "model.hpp"
#include "sector.hpp"
#include "dummy.hpp"
#include "logger.hpp"
#include "vfs.hpp"

#include <filesystem>

[[nodiscard]] glm::mat4 getMatrixFromMesh(MFFormat::DataFormat4DS::Mesh& mesh) {
    glm::vec3 meshPos = { mesh.mPos.x, mesh.mPos.y, mesh.mPos.z };
    glm::vec3 meshScale = { mesh.mScale.x, mesh.mScale.y, mesh.mScale.z };
    glm::quat meshRot;

    meshRot.w = mesh.mRot.w;
    meshRot.x = mesh.mRot.x;
    meshRot.y = mesh.mRot.y;
    meshRot.z = mesh.mRot.z;

    const auto translation = glm::translate(glm::mat4(1.f), meshPos);
    const auto scale = glm::scale(glm::mat4(1.f), meshScale);
    const auto rot = glm::mat4(1.f) * glm::toMat4(meshRot);
    const auto world = translation * rot * scale;
    return world;
}

[[nodiscard]] std::vector<std::string> makeAnimationNames(const std::string& baseFileName, unsigned int frames) {
    std::vector<std::string> result;
    if (baseFileName.length() == 0) {
        for (unsigned int i = 0; i < frames; ++i) result.push_back("");
        return result;
    }

    size_t dotPos = baseFileName.find('.');
    std::string preStr, postStr;

    postStr = baseFileName.substr(dotPos);
    preStr = baseFileName.substr(0, dotPos - 2);

    for (unsigned int i = 0; i < frames; ++i) {
        std::string numStr = std::to_string(i);
        if (numStr.length() == 1) numStr = "0" + numStr;
        result.push_back(preStr + numStr + postStr);
    }

    return result;
}

[[nodiscard]] std::shared_ptr<Material> loadMaterial(const MFFormat::DataFormat4DS::Material& mafiaMaterial) {
    auto material = std::make_shared<Material>();

    bool hasDiffuse = mafiaMaterial.mFlags & MFFormat::DataFormat4DS::MaterialFlag::MATERIALFLAG_TEXTUREDIFFUSE;
    bool isDiffuseAnimated = mafiaMaterial.mFlags & MFFormat::DataFormat4DS::MaterialFlag::MATERIALFLAG_ANIMATEDTEXTUREDIFFUSE;

    bool hasAlpha = mafiaMaterial.mFlags & MFFormat::DataFormat4DS::MaterialFlag::MATERIALFLAG_ALPHATEXTURE;
    bool isAlphaAnimated = mafiaMaterial.mFlags & MFFormat::DataFormat4DS::MaterialFlag::MATERIALFLAG_ANIMATEXTEXTUREALPHA;

    bool isColored = mafiaMaterial.mFlags & MFFormat::DataFormat4DS::MaterialFlag::MATERIALFLAG_COLORED;
    bool hasEnvMap = mafiaMaterial.mFlags & MFFormat::DataFormat4DS::MaterialFlag::MATERIALFLAG_ENVIRONMENTMAP;
    bool isDoubleSided = mafiaMaterial.mFlags & MFFormat::DataFormat4DS::MaterialFlag::MATERIALFLAG_DOUBLESIDEDMATERIAL;
    bool normalBlending = mafiaMaterial.mFlags & MFFormat::DataFormat4DS::MaterialFlag::MATERIALFLAG_NORMALTEXTUREBLEND;
    bool mulBlending = mafiaMaterial.mFlags & MFFormat::DataFormat4DS::MaterialFlag::MATERIALFLAG_MULTIPLYTEXTUREBLEND;
    bool addBlending = mafiaMaterial.mFlags & MFFormat::DataFormat4DS::MaterialFlag::MATERIALFLAG_ADDITIVETEXTUREBLEND;
    bool hasTransparencyKey = mafiaMaterial.mFlags & MFFormat::DataFormat4DS::MaterialFlag::MATERIALFLAG_COLORKEY;
    //bool hasAditiveMixing = mafiaMaterial.mFlags & MFFormat::DataFormat4DS::MaterialFlag::MATERIALFLAG_ADDITIVEMIXING;

    if (hasDiffuse) {
        material->setHasTransparencyKey(hasTransparencyKey);
        material->createDiffuseTexture(mafiaMaterial.mDiffuseMapName);
        material->setKind(hasTransparencyKey ? MaterialKind::CUTOUT : MaterialKind::DIFFUSE);
    }

    if (hasAlpha) {
        material->createAlphaTexture(mafiaMaterial.mAlphaMapName);
        material->setKind(MaterialKind::ALPHA);
    }

    if (isDiffuseAnimated) {
        material->setHasTransparencyKey(hasTransparencyKey);
        if (isAlphaAnimated) {
            auto diffuseAnimationNames = makeAnimationNames(mafiaMaterial.mDiffuseMapName, mafiaMaterial.mAnimSequenceLength);
            auto alphaAnimationNames = makeAnimationNames(mafiaMaterial.mAlphaMapName, mafiaMaterial.mAnimSequenceLength);

            for (size_t i = 0; i < diffuseAnimationNames.size(); ++i) {
                material->appendAnimatedTexture(diffuseAnimationNames[i].c_str());
            }
        } else {
            auto diffuseAnimationNames = makeAnimationNames(mafiaMaterial.mDiffuseMapName, mafiaMaterial.mAnimSequenceLength);
            for (size_t i = 0; i < diffuseAnimationNames.size(); ++i) {
                material->appendAnimatedTexture(diffuseAnimationNames[i].c_str());
            }
        }

        material->setAnimationPeriod(mafiaMaterial.mFramePeriod);
    }

    if (hasEnvMap) {
        if (normalBlending) {
            material->setTextureBlending(TextureBlending::NORMAL);
            if (hasEnvMap) {
                material->setEnvRatio(mafiaMaterial.mEnvRatio);
            }
        }

        if (mulBlending) {
            material->setTextureBlending(TextureBlending::MUL);
        }

        if (addBlending) {
            material->setTextureBlending(TextureBlending::ADD);
        }

        material->createEnvTexture(mafiaMaterial.mEnvMapName);
        material->setKind(MaterialKind::ENV);
    }

    material->setTransparency(mafiaMaterial.mTransparency);
    material->setDoubleSided(isDoubleSided);
    //material->setAditiveMixing(hasAditiveMixing);
    material->setAmbient({mafiaMaterial.mAmbient.x, mafiaMaterial.mAmbient.y, mafiaMaterial.mAmbient.z});
    material->setEmission({mafiaMaterial.mEmission.x, mafiaMaterial.mEmission.y, mafiaMaterial.mEmission.z});

    if (!hasDiffuse || isColored) {
        material->setDiffuse({mafiaMaterial.mDiffuse.x, mafiaMaterial.mDiffuse.y, mafiaMaterial.mDiffuse.z});
    }

    //material->init();
    return material;
}

std::shared_ptr<Mesh> loadStandard(MFFormat::DataFormat4DS::Mesh& mesh,
    const std::vector<MFFormat::DataFormat4DS::Material>& materials) {
    std::vector<MFFormat::DataFormat4DS::Lod>* lods = nullptr;
    bool isStaticMesh = false;

    std::shared_ptr<Mesh> newMesh = nullptr;

    switch (mesh.mVisualMeshType) {
    case MFFormat::DataFormat4DS::VisualMeshType::VISUALMESHTYPE_STANDARD: {
        lods = &mesh.mStandard.mLODs;
    } break;
    case MFFormat::DataFormat4DS::VisualMeshType::VISUALMESHTYPE_BILLBOARD: {
        lods = &mesh.mBillboard.mStandard.mLODs;
        isStaticMesh = false;
        newMesh = std::make_shared<Billboard>();
    } break;
    case MFFormat::DataFormat4DS::VisualMeshType::VISUALMESHTYPE_SINGLEMESH: {
        lods = &mesh.mSingleMesh.mStandard.mLODs;
    } break;
    case MFFormat::DataFormat4DS::VisualMeshType::VISUALMESHTYPE_SINGLEMORPH: {
        lods = &mesh.mSingleMorph.mSingleMesh.mStandard.mLODs;
    } break;
    default: {
        //Logger::get().warn("unable to load visual mesh type {}",  mesh.mVisualMeshType);
    } break;
    }

    if(newMesh == nullptr) {
        newMesh = std::make_shared<Mesh>();
    }

    newMesh->setName(mesh.mMeshName);
    newMesh->setMatrix(getMatrixFromMesh(mesh));
    newMesh->setStatic(isStaticMesh);

    if (!lods || lods->size() <= 0) {
        return newMesh;
    }

    auto& lod = lods->at(0);

    // NOTE: get vertices from lod
    {
        std::vector<Vertex> vertices;
        for (const auto& mafiaVertex : lod.mVertices) {
            Vertex vertex{};
            vertex.p = { mafiaVertex.mPos.x, mafiaVertex.mPos.y, mafiaVertex.mPos.z };
            vertex.n = { mafiaVertex.mNormal.x, mafiaVertex.mNormal.y, mafiaVertex.mNormal.z };
            vertex.uv = { mafiaVertex.mUV.x, mafiaVertex.mUV.y * -1.0f };
            vertices.push_back(vertex);
        }

        newMesh->setVertices(vertices);
    }

    //NOTE: load face groups
    {
        for (const auto& mafiaFaceGroup : lod.mFaceGroups) {
            std::vector<uint16_t> indices;
            for (const auto& face : mafiaFaceGroup.mFaces) {
                indices.push_back(face.mA);
                indices.push_back(face.mB);
                indices.push_back(face.mC);
            }

            auto faceGroup = std::make_unique<FaceGroup>(indices, newMesh);
            if (mafiaFaceGroup.mMaterialID > 0) {
                auto loadedMat = loadMaterial(materials[mafiaFaceGroup.mMaterialID - 1]);
                
                //NOTE: set material kind to billboard
                if (newMesh->getType() == FrameType::BILLBOARD) {
                    loadedMat->setKind(MaterialKind::BILLBOARD);
                }

                faceGroup->setMaterial(std::move(loadedMat));
            }
            newMesh->addFaceGroup(std::move(faceGroup));
        }
    }

    return newMesh;
}

[[nodiscard]] std::shared_ptr<Dummy> loadDummy(MFFormat::DataFormat4DS::Mesh& mesh) {
    auto newMesh = std::make_shared<Dummy>();
    newMesh->setName(mesh.mMeshName);
    newMesh->setMatrix(getMatrixFromMesh(mesh));
    auto bbox = std::make_pair<glm::vec3, glm::vec3>({mesh.mDummy.mMinBox.x, mesh.mDummy.mMinBox.y, mesh.mDummy.mMinBox.z},
                                                               {mesh.mDummy.mMaxBox.x, mesh.mDummy.mMaxBox.y, mesh.mDummy.mMaxBox.z});
    newMesh->setBBOX(bbox);
    return newMesh;
}

[[nodiscard]] std::shared_ptr<Sector> loadSector(MFFormat::DataFormat4DS::Mesh& mesh) {
    auto newMesh = std::make_shared<Sector>();
    newMesh->setName(mesh.mMeshName);
    newMesh->setMatrix(getMatrixFromMesh(mesh));
    auto bbox = std::make_pair<glm::vec3, glm::vec3>({mesh.mSector.mMinBox.x, mesh.mSector.mMinBox.y, mesh.mSector.mMinBox.z},
                                                    {mesh.mSector.mMaxBox.x, mesh.mSector.mMaxBox.y, mesh.mSector.mMaxBox.z});
    newMesh->setBBOX(bbox);
    return newMesh;
}

std::shared_ptr<Frame> meshFactory(MFFormat::DataFormat4DS::Mesh& mesh, const std::vector<MFFormat::DataFormat4DS::Material>& materials) {
    switch (mesh.mMeshType) {
        case MFFormat::DataFormat4DS::MeshType::MESHTYPE_STANDARD: {
            return loadStandard(mesh, materials);
        } break;

        case MFFormat::DataFormat4DS::MeshType::MESHTYPE_SECTOR: {
            return loadSector(mesh);
        } break;
        
        case MFFormat::DataFormat4DS::MeshType::MESHTYPE_COLLISION:
        case MFFormat::DataFormat4DS::MeshType::MESHTYPE_DUMMY: {
            return loadDummy(mesh);
        } break;

        default: {
            //Logger::get().warn("unable to load mesh type {}", mesh.mMeshName);
        } break;
    }

    return nullptr;
}

std::shared_ptr<Model> ModelLoader::loadModel(const std::string& path) { 
    auto modelFile = Vfs::getFile(path);
    if (!modelFile.size()) {
        Logger::get().error("unable to load model {}", path);
        return nullptr;
    }

    MFFormat::DataFormat4DS modelParser;
    if (!modelParser.load(modelFile)) {
        Logger::get().error("unable to load model {}", path);
        return nullptr;
    }

    auto parserModel = modelParser.getModel();
    auto modelName = std::filesystem::path(path).filename().string();
    auto model = std::make_shared<Model>();
    model->setName(modelName);

    // NOTE: linear loading of meshes
    std::vector<std::shared_ptr<Frame>> loadedMeshes;
    for (auto& mesh : parserModel.mMeshes) {
        auto loadedMesh = meshFactory(mesh, parserModel.mMaterials);
        if (loadedMesh) {
            loadedMeshes.push_back(loadedMesh);
        }
    }

    // NOTE: build tree based on parent ids
    for (size_t i = 0; i < loadedMeshes.size(); i++) {
        auto currentMesh = loadedMeshes[i];
        auto currentMafiaMesh = parserModel.mMeshes[i];

        if (currentMafiaMesh.mParentID > 0) {
            auto parentNode = loadedMeshes[currentMafiaMesh.mParentID - 1];
            parentNode->addChild(currentMesh);
        }
        else {
            model->addChild(std::move(currentMesh));
        }
    }
    
    return model;
}
