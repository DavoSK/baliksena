#pragma once
#include <string>
#include "logger.hpp"
#include "mafia/parser_5ds.hpp"
#include "single_mesh.hpp"

enum class Key 
{
    Trans,
    Rot,
    Scale,
    Count
};

class Sequence {
public:
    Sequence(Frame* frame) : 
        mFrame(frame) 
    {
        assert(mFrame != nullptr);
        start();
    }

    void update() {
        if(!Translations.empty()) {
            const auto& mov = Translations.at(TranslationKeyId);
            mFrame->setPos({ mov.x, mov.y, mov.z });

            if(TranslationKeyId + 1 < Translations.size())
                TranslationKeyId++;
            else 
                Finished[0] = true;
        }

        if(!Rotations.empty()) {
            const auto& rot = Rotations.at(RotationKeyId);
            glm::quat rotation{};
            rotation.w = rot.w;
            rotation.x = rot.x;
            rotation.y = rot.y;
            rotation.z = rot.z;
            mFrame->setRot(rotation);

            if(RotationKeyId + 1 < Rotations.size())
                RotationKeyId++;
            else 
                Finished[1] = true;
        }

        if(!Scales.empty()) {
            const auto& scl = Scales.at(ScaleKeyId);
            mFrame->setScale({ scl.x, scl.y, scl.z});

            if(ScaleKeyId + 1 < Scales.size())
                ScaleKeyId++;
            else 
                Finished[2] = true;
        }
    }

    bool isFinished() const {
        return (Finished[0] && Finished[1] && Finished[2]);
    }

    void start() {
        TranslationKeyId = 0;
        RotationKeyId = 0;
        ScaleKeyId = 0;

        Finished[0] = Translations.empty();
        Finished[1] = Rotations.empty();
        Finished[2] = Scales.empty();
    }

    Frame* mFrame;
    std::vector<glm::vec3> Translations{};
    std::vector<glm::quat> Rotations{};
    std::vector<glm::vec3> Scales{};

    int32_t TranslationKeyId = 0;
    int32_t RotationKeyId = 0;
    int32_t ScaleKeyId = 0;
    bool Finished[3];
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