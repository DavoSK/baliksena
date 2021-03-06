#include "model_loader.hpp"
#include "mafia/parser_4ds.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "renderer.hpp"
#include "mesh.hpp"
#include "single_mesh.hpp"
#include "billboard.hpp"
#include "material.hpp"
#include "texture.hpp"
#include "model.hpp"
#include "sector.hpp"
#include "dummy.hpp"
#include "logger.hpp"
#include "vfs.hpp"

#include <filesystem>

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
    bool hasMipmaps = mafiaMaterial.mFlags & MFFormat::DataFormat4DS::MaterialFlag::MATERIALFLAG_MIPMAPPING;
    
    //NOTE: set mipmaps before we load material texture :)
    material->setMipmaps(hasMipmaps);
    //bool hasAditiveMixing = mafiaMaterial.mFlags & MFFormat::DataFormat4DS::MaterialFlag::MATERIALFLAG_ADDITIVEMIXING;

    if (hasDiffuse) {
        material->setHasTransparencyKey(hasTransparencyKey);
        material->createDiffuseTexture(mafiaMaterial.mDiffuseMapName);
        material->setKind(hasTransparencyKey ? Renderer::MaterialKind::CUTOUT : Renderer::MaterialKind::DIFFUSE);
    }

    if (hasAlpha) {
        material->createAlphaTexture(mafiaMaterial.mAlphaMapName);
        material->setKind(Renderer::MaterialKind::ALPHA);
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
            material->setTextureBlending(Renderer::TextureBlending::NORMAL);
            if (hasEnvMap) {
                material->setEnvRatio(mafiaMaterial.mEnvRatio);
            }
        }

        if (mulBlending) {
            material->setTextureBlending(Renderer::TextureBlending::MUL);
        }

        if (addBlending) {
            material->setTextureBlending(Renderer::TextureBlending::ADD);
        }

        material->createEnvTexture(mafiaMaterial.mEnvMapName);
        material->setKind(Renderer::MaterialKind::ENV);
    }

    material->setTransparency(mafiaMaterial.mTransparency);
    material->setDoubleSided(isDoubleSided);

    //material->setAditiveMixing(hasAditiveMixing);
    material->setAmbient({mafiaMaterial.mAmbient.x, mafiaMaterial.mAmbient.y, mafiaMaterial.mAmbient.z});
    material->setEmission({mafiaMaterial.mEmission.x, mafiaMaterial.mEmission.y, mafiaMaterial.mEmission.z});
    
    if(isColored || !hasDiffuse) {
        material->setDiffuse({mafiaMaterial.mDiffuse.x, mafiaMaterial.mDiffuse.y, mafiaMaterial.mDiffuse.z});
    }

    //material->init();
    return material;
}

std::shared_ptr<Mesh> loadStandard(MFFormat::DataFormat4DS::Mesh& mesh,
    const std::vector<MFFormat::DataFormat4DS::Material>& materials) {
    std::vector<MFFormat::DataFormat4DS::Lod>* lods = nullptr;
    std::shared_ptr<Mesh> newMesh = nullptr;
    switch (mesh.mVisualMeshType) {
        case MFFormat::DataFormat4DS::VisualMeshType::VISUALMESHTYPE_STANDARD: {
            lods = &mesh.mStandard.mLODs;
        } break;
        case MFFormat::DataFormat4DS::VisualMeshType::VISUALMESHTYPE_BILLBOARD: {
            lods = &mesh.mBillboard.mStandard.mLODs;
            newMesh = std::make_shared<Billboard>();
        } break;
        case MFFormat::DataFormat4DS::VisualMeshType::VISUALMESHTYPE_SINGLEMESH: {
            lods = &mesh.mSingleMesh.mStandard.mLODs;
            //newMesh = std::make_shared<SingleMesh>();
        } break;
        case MFFormat::DataFormat4DS::VisualMeshType::VISUALMESHTYPE_SINGLEMORPH: {
            lods = &mesh.mSingleMorph.mSingleMesh.mStandard.mLODs;
            newMesh = std::make_shared<SingleMesh>();            
        } break;
        default: {
            //Logger::get().warn("unable to load visual mesh type {}",  mesh.mVisualMeshType);
        } break;
    }

    if(newMesh == nullptr) {
        newMesh = std::make_shared<Mesh>();
    }

    newMesh->setName(mesh.mMeshName);
    newMesh->setPos({mesh.mPos.x, mesh.mPos.y, mesh.mPos.z});
    newMesh->setScale({mesh.mScale.x, mesh.mScale.y, mesh.mScale.z});

    //NOTE: flip quaterion ( ls3d retardness )
    glm::quat meshRot;
    meshRot.w = mesh.mRot.w;
    meshRot.x = mesh.mRot.x;
    meshRot.y = mesh.mRot.y;
    meshRot.z = mesh.mRot.z;
    newMesh->setRot(meshRot);
    
    if (!lods || lods->size() <= 0) {
        return newMesh;
    }

    auto& lod = lods->at(0);

    // NOTE: get vertices from lod
    std::vector<Renderer::Vertex> vertices;
    {
        
        for (const auto& mafiaVertex : lod.mVertices) {
            Renderer::Vertex vertex{};
            vertex.p = { mafiaVertex.mPos.x, mafiaVertex.mPos.y, mafiaVertex.mPos.z };
            vertex.n = { mafiaVertex.mNormal.x, mafiaVertex.mNormal.y, mafiaVertex.mNormal.z };
            vertex.uv = { mafiaVertex.mUV.x, mafiaVertex.mUV.y * -1.0f };
            vertices.push_back(vertex);
        }
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
                if (newMesh->getFrameType() == FrameType::Billboard) {
                    loadedMat->setKind(Renderer::MaterialKind::BILLBOARD);
                }

                faceGroup->setMaterial(std::move(loadedMat));
            }
            newMesh->addFaceGroup(std::move(faceGroup));
        }
    }

    //NOTE: load bone for singlemeshes
    if((mesh.mVisualMeshType == MFFormat::DataFormat4DS::VisualMeshType::VISUALMESHTYPE_SINGLEMESH || 
       mesh.mVisualMeshType == MFFormat::DataFormat4DS::VisualMeshType::VISUALMESHTYPE_SINGLEMORPH) && 
       mesh.mSingleMorph.mSingleMesh.mLODs.size() > 0) {
        
        auto singleMesh = std::dynamic_pointer_cast<SingleMesh>(newMesh);
        const auto& singleLOD = mesh.mSingleMorph.mSingleMesh.mLODs.at(0);
        
        std::vector<Bone> singleMeshBones;
    
        size_t skipVertices = 0;   
        for(size_t i = 0; i < singleLOD.mBones.size(); i++) {
            const auto& lodBone = singleLOD.mBones[i];

            Bone newBone {};
            newBone.mBoneID                 = lodBone.mBoneID;
            newBone.mInverseTransform       = *(const glm::mat4*)&lodBone.mInverseTransform;
            newBone.mMaxBox                 = { lodBone.mMaxBox.x, lodBone.mMaxBox.y, lodBone.mMaxBox.z };
            newBone.mMinBox                 = { lodBone.mMinBox.x, lodBone.mMinBox.y, lodBone.mMinBox.z };
            newBone.mNoneWeightedVertCount  = lodBone.mNoneWeightedVertCount;
            newBone.mWeightedVertCount      = lodBone.mWeightedVertCount;
            newBone.mWeights                = lodBone.mWeights;
                
            //NOTE: append 1.0f weights
            for (uint32_t j = 0; j < newBone.mNoneWeightedVertCount; j++) {
                Renderer::Vertex& vertexToModify = vertices[skipVertices + j];
                vertexToModify.indexes = { (float)i, (float)newBone.mBoneID };
                vertexToModify.weights = { 1.0f, 0.0f };
            }

            skipVertices += newBone.mNoneWeightedVertCount;

            //NOTE: append rest of weights
            for (uint32_t j = 0; j < newBone.mWeights.size(); j++) {
                Renderer::Vertex& vertexToModify = vertices[skipVertices + j];
                vertexToModify.indexes = { (float)i, (float)newBone.mBoneID };
                vertexToModify.weights = { newBone.mWeights[j], 1.0f - newBone.mWeights[j] };
            }

            skipVertices += newBone.mWeights.size();
            singleMeshBones.push_back(newBone);
        }
        
        //NOTE: append rest of weights of 1.0f
        for (uint32_t j = 0; j < singleLOD.mNonWeightedVertCount; j++) {
            Renderer::Vertex& vertexToModify = vertices[skipVertices + j];
            vertexToModify.indexes = { (float)0, (float)0 };
            vertexToModify.weights = { 1.0f, 0.0f };
        }

        singleMesh->setBones(singleMeshBones);        
    }

    newMesh->setVertices(vertices);
    return newMesh;
}

[[nodiscard]] std::shared_ptr<Dummy> loadDummy(MFFormat::DataFormat4DS::Mesh& mesh) {
    auto newMesh = std::make_shared<Dummy>();
    newMesh->setName(mesh.mMeshName);
    newMesh->setPos({mesh.mPos.x, mesh.mPos.y, mesh.mPos.z});
    newMesh->setScale({mesh.mScale.x, mesh.mScale.y, mesh.mScale.z});

    //NOTE: flip quaterion ( ls3d retardness )
    glm::quat meshRot;
    meshRot.w = mesh.mRot.w;
    meshRot.x = mesh.mRot.x;
    meshRot.y = mesh.mRot.y;
    meshRot.z = mesh.mRot.z;
    newMesh->setRot(meshRot);
    
    auto bbox = std::make_pair<glm::vec3, glm::vec3>({mesh.mDummy.mMinBox.x, mesh.mDummy.mMinBox.y, mesh.mDummy.mMinBox.z},
                                                               {mesh.mDummy.mMaxBox.x, mesh.mDummy.mMaxBox.y, mesh.mDummy.mMaxBox.z});
    newMesh->setBBOX(bbox);
    return newMesh;
}

[[nodiscard]] std::shared_ptr<Sector> loadSector(MFFormat::DataFormat4DS::Mesh& mesh) {
    auto newMesh = std::make_shared<Sector>();
    newMesh->setName(mesh.mMeshName);
    newMesh->setPos({mesh.mPos.x, mesh.mPos.y, mesh.mPos.z});
    newMesh->setScale({mesh.mScale.x, mesh.mScale.y, mesh.mScale.z});

    //NOTE: flip quaterion ( ls3d retardness )
    glm::quat meshRot;
    meshRot.w = mesh.mRot.w;
    meshRot.x = mesh.mRot.x;
    meshRot.y = mesh.mRot.y;
    meshRot.z = mesh.mRot.z;
    newMesh->setRot(meshRot);

    auto bbox = std::make_pair<glm::vec3, glm::vec3>({mesh.mSector.mMinBox.x, mesh.mSector.mMinBox.y, mesh.mSector.mMinBox.z},
                                                    {mesh.mSector.mMaxBox.x, mesh.mSector.mMaxBox.y, mesh.mSector.mMaxBox.z});
    newMesh->setBBOX(bbox);
    return newMesh;
}

[[nodiscard]] std::shared_ptr<Joint> loadJoint(MFFormat::DataFormat4DS::Mesh& mesh) {
    auto newMesh = std::make_shared<Joint>();
    
    newMesh->setName(mesh.mMeshName);
    newMesh->setPos({mesh.mPos.x, mesh.mPos.y, mesh.mPos.z});
    newMesh->setScale({mesh.mScale.x, mesh.mScale.y, mesh.mScale.z});

    //NOTE: flip quaterion ( ls3d retardness )
    glm::quat meshRot;
    meshRot.w = mesh.mRot.w;
    meshRot.x = mesh.mRot.x;
    meshRot.y = mesh.mRot.y;
    meshRot.z = mesh.mRot.z;
    newMesh->setRot(meshRot);

    newMesh->setName(mesh.mMeshName);
    newMesh->setBoneId(mesh.mJoint.mJointID);
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
        
        case MFFormat::DataFormat4DS::MeshType::MESHTYPE_JOINT: {
            return loadJoint(mesh);
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

std::shared_ptr<Model> ModelLoader::loadModel(const std::string& path, const std::string& modelName) { 
    auto modelFile = Vfs::getFile(path);
    if (!modelFile.has_value()) {
        Logger::get().error("unable to load model {}", path);
        return nullptr;
    }

    MFFormat::DataFormat4DS modelParser;
    if (!modelParser.load(modelFile.value())) {
        Logger::get().error("unable to load model {}", path);
        return nullptr;
    }

    auto parserModel = modelParser.getModel();
    //auto modelName = std::filesystem::path(path).filename().replace_extension("").string();
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

        if(modelName.find("scene") == std::string::npos) {
            currentMesh->setName(modelName + "." + currentMesh->getName());
        }

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
