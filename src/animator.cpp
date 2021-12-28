#include "animator.hpp"
#include "single_mesh.hpp"

#include "vfs.hpp"
#include "logger.hpp"

#include <sokol/sokol_time.h>

Animator::Animator(SingleMesh* mesh) 
    : mSingleMesh(mesh) {
}

bool Animator::open(const std::string& path) {
    auto file = Vfs::getFile("ANIMS\\" + path);
    if(!file.has_value()) {
        Logger::get().error("unable to open animation file: {} !", path);
        return false;
    }

    if(!mParsedAnim.load(file.value())) {
        Logger::get().error("unable to parse animation file: {} !", path);
        return false;
    }

    return true;
}

void Animator::init() {
    //NOTE: init sequences
    for(const auto& sequence : mParsedAnim.getSequences()) {
        const auto& modelName = mSingleMesh->getOwner()->getName();
        auto targetFrame = mSingleMesh->getOwner()->findFrame(modelName + "." + sequence.getName());
        if(targetFrame != nullptr) {
           
            Sequence newSequence(targetFrame.get());

            for(const auto& mov : sequence.getTranslations()) {
                newSequence.Translations.push_back({ mov.x, mov.y, mov.z });
            }

            for(const auto& rot : sequence.getRotations()) {
                glm::quat rotation{};
                rotation.w = rot.w;
                rotation.x = rot.x;
                rotation.y = rot.y;
                rotation.z = rot.z;
                newSequence.Rotations.push_back(rotation);
            }
            
            for(const auto& scl : sequence.getScales())  {
                newSequence.Scales.push_back({ scl.x, scl.y, scl.z });
            }

            mSequences.push_back(newSequence);
        } else {
            Logger::get().error("Unable to get bone: {}", modelName + "." + sequence.getName());
            return;
        }
    }

    mInited = true;
    mLastFrameTime = stm_now();
}

void Animator::update() {
    if(!mInited) {
        init();
        return;
    }

    uint64_t diff = stm_ms(stm_diff(stm_now(), mLastFrameTime));
    if(diff > 33) {
        size_t finishedCnt = 0;
        for(auto& seq : mSequences) {
            if(seq.isFinished())
                finishedCnt++;

            seq.update();
        }

        //NOTE: if all sequences are done entire animation is finished
        //we cam start again then :)
        if(finishedCnt == mSequences.size()) {
            for(auto& seq : mSequences) {
                seq.start();
            }
        }

        mLastFrameTime = stm_now();
    }
}