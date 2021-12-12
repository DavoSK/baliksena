#include "cutscene.hpp"
#include "vfs.hpp"
#include "scene.hpp"
#include "camera.h"
#include "app.hpp"

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
            mDefinitions[i] = App::get()->getScene()->findFrame(animObj.frameName);
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
        if(chunk.timestamp <= diff) {
            focusChunk = chunk;
        }
    }

    auto* cam = App::get()->getScene()->getActiveCamera();
    cam->setViewMatrix(glm::lookAtLH(tsChunk.position, focusChunk.position, cam->Up));
    cam->setProjMatrix(glm::perspectiveLH(tsChunk.fov, cam->Aspect, cam->Near, cam->Far));  
}

void Cutscene::updateObjects() {
    // uint64_t diff = stm_ms(stm_diff(stm_now(), mStartTime));

    // RepFile::Transformation tsChunk;
    // for(const auto& obj : mRep.transformBlocks) {
    //     if(tsChunk.header.timestamp <= diff) {
    //         tsChunk = obj;
    //     }
    // }

    // std::shared_ptr<Frame> frame = mDefinitions[tsChunk.payload.getAnimationID()];
    // if(frame != nullptr) {
    //     frame->setPos(tsChunk.payload.position);
    // }
}

void Cutscene::tick() {
    updateCamera();
    updateObjects();
}
    
