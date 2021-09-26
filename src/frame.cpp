#include "frame.hpp"

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
    //updateAABBWorld();
    for (const auto& frame : mChilds) {
        frame->invalidateTransformRecursively();
    }
}
