#pragma once
#include <string>
#include "logger.hpp"
#include "mafia/parser_5ds.hpp"
#include "single_mesh.hpp"

class Sequence {
public:
    Sequence(Frame* frame) : 
        mFrame(frame) 
    {
        assert(mFrame != nullptr);
    }

    void update() {
        glm::mat4 translation(1.0f);
        if(!Translations.empty()) {
            const auto& mov = Translations.at(TranslationKeyId);
            translation = glm::translate(glm::mat4(1.0f), { mov.x, mov.y, mov.z });

            if(TranslationKeyId + 1 < Translations.size())
                TranslationKeyId++;
        }

        glm::mat4 rotationMat(1.0f);
        if(!Rotations.empty()) {
            const auto& rot = Rotations.at(RotationKeyId);
            glm::quat rotation{};
            rotation.w = rot.w;
            rotation.x = rot.x;
            rotation.y = rot.y;
            rotation.z = rot.z;
            rotationMat = glm::toMat4(rotation);

            if(RotationKeyId + 1 < Rotations.size())
                RotationKeyId++;
        }

        glm::mat4 scale(1.0f);
        if(!Scales.empty()) {
            const auto& scl = Scales.at(ScaleKeyId);
            scale = glm::scale(glm::mat4(1.0f), { scl.x, scl.y, scl.z});

            if(ScaleKeyId + 1 < Scales.size())
                ScaleKeyId++;
        }

        glm::mat4 trasnMatrix = translation * rotationMat * scale;
        if(mFrame->getFrameType() == FrameType::Joint) {
            ((Joint*)mFrame)->setLocalMatrix(trasnMatrix);
        } else {
            mFrame->setMatrix(trasnMatrix);
        }
    }

    bool isFinished() {
        auto transformFinished =    TranslationKeyId    >=  Translations.size();
        auto rotationFinished =     RotationKeyId       >=  Rotations.size();
        auto scaleFinished =        ScaleKeyId          >=  Scales.size();
        return (transformFinished && rotationFinished && scaleFinished);
    }

    void start() {
        TranslationKeyId = 0;
        RotationKeyId = 0;
        ScaleKeyId = 0;
    }

    Frame* mFrame;
    std::vector<glm::vec3> Translations;
    std::vector<glm::quat> Rotations;
    std::vector<glm::vec3> Scales;

    int32_t TranslationKeyId = 0;
    int32_t RotationKeyId = 0;
    int32_t ScaleKeyId = 0;
};

class Animator {
public:
    Animator(SingleMesh* mesh);
    bool open(const std::string& path);
    void update();
    void init();
private:
    bool mInited = false;
    int mSequenceId = 0;
    

    uint64_t mLastFrameTime = 0;
    MFFormat::DataFormat5DS mParsedAnim{};
    std::vector<Sequence> mSequences;
    SingleMesh* mSingleMesh{ nullptr };
};