#include "cutscene.hpp"
#include "vfs.hpp"
#include "scene.hpp"
#include "camera.h"
#include "app.hpp"
#include "logger.hpp"

#include <optional>
#include <sokol/sokol_time.h>

void Cutscene::start(const std::string& recordName) {
    auto inputFile = Vfs::getFile("RECORDS\\" + recordName);
    if(inputFile.has_value()) {
        RepFile::Loader loader;
        mRep = loader.loadFile(inputFile.value());
        mStartTime = stm_now();
        
        //NOTE: find animated objects
        for(size_t i = 0; i < mRep.animatedObjects.size(); i++) {
            const auto& animObj = mRep.animatedObjects[i];
            
            mDefinitions[animObj.frameName] = App::get()->getScene()->findFrame(animObj.frameName);
            Logger::get().info("frm def {} {}", i, animObj.frameName);
        }
    }
}

void Cutscene::updateCamera() {
    uint64_t diff = stm_ms(stm_diff(stm_now(), mStartTime));
    
    RepFile::CameraTransformationChunk tsChunk;
    for(const auto& chunk : mRep.cameraPositionChunks) {
        if(chunk.timestamp <= diff) {
            tsChunk = chunk;
        }
    }

    RepFile::CameraFocusChunk focusChunk;
    for(const auto& chunk : mRep.camerafocusChunks) {
        if(chunk.timestamp >= diff) {
            focusChunk = chunk;
        }
    }

    auto* cam = App::get()->getScene()->getActiveCamera();
    cam->setViewMatrix(glm::lookAtLH(tsChunk.position, focusChunk.position, cam->Up));
    cam->setProjMatrix(glm::perspectiveLH(tsChunk.fov, cam->Aspect, cam->Near, cam->Far));  
}



void Cutscene::updateObjects() {
    uint64_t diff = stm_ms(stm_diff(stm_now(), mStartTime));

    auto getAnimationById = [&](size_t animationId) -> std::string { 
        for(auto& animBlock : mRep.animationBlocks) {
            if(animBlock.animationID == animationId) 
                return animBlock.animationName;
        }

        return {};
    };

    auto playTransform = [&](RepFile::Transformation& transform) -> void {
        if(transform.payload.hasAnimationID()) {
            Logger::get().info("anim: {} for: {}: flags: {}", getAnimationById(transform.payload.getAnimationID()), transform.def.frameName, transform.payload.getFlagsAsString());
        } else {
            Logger::get().info("transform for: {}: flags: {}", transform.def.frameName, transform.payload.getFlagsAsString());
        }
    };

    static size_t curentTransformBlockIdx = 0;
    for( size_t i = curentTransformBlockIdx; i < mRep.transformBlocks.size(); i++) {
        auto& currentTransformBlock = mRep.transformBlocks[i];
        if(diff >= currentTransformBlock.header.timestamp) {
            playTransform(currentTransformBlock);
            curentTransformBlockIdx = i + 1;
        }
    }
}

void Cutscene::tick() {
    //updateCamera();
    updateObjects();
}
    
