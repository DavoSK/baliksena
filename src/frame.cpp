#include "frame.hpp"
#include <glm/ext.hpp>

void Frame::render() {
    if (!mOn) return;

    for (auto& child : mChilds) {
        child->render();
    }
}

void Frame::addChild(std::shared_ptr<Frame> frame) {
    frame->setOwner(this);
    mChilds.push_back(std::move(frame));
}

const glm::mat4& Frame::getWorldMatrix() {
    if (!mIsTransformDirty) {
        return mCachedTransform;
    }

    if (getOwner() == nullptr) {
        return mTransform;
    }

    mCachedTransform = getOwner()->getWorldMatrix() * mTransform;
    mIsTransformDirty = false;
    return mCachedTransform;
}

void Frame::invalidateTransformRecursively() {
    invalidateTransform();
    updateAABBWorld();
    
    for (const auto& frame : mChilds) {
        frame->invalidateTransformRecursively();
    }
}

void Frame::setBBOX(const std::pair<glm::vec3, glm::vec3>& bbox) {
    mAABB = bbox;
    updateAABBWorld();
}

void Frame::updateAABBWorld() {
    const auto currentWorld = getWorldMatrix();
    const auto min = glm::translate(currentWorld, mAABB.first);
    const auto max = glm::translate(currentWorld, mAABB.second);
    mABBBWorld = std::make_pair<glm::vec3, glm::vec3>(min[3], max[3]);
}