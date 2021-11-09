#include "frame.hpp"
#include <glm/gtc/matrix_transform.hpp>

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

void Frame::removeChild(std::shared_ptr<Frame> frame) {
    frame->setOwner(nullptr);
    mChilds.erase(std::remove(mChilds.begin(), mChilds.end(), frame), mChilds.end());
}

const glm::mat4& Frame::getWorldMatrix() {
    if (!mIsTransformDirty) {
        return mCachedTransform;
    }

    if (getOwner() == nullptr) {
        mCachedTransform = mTransform;
        return mTransform;
    }

    mCachedTransform = getOwner()->getWorldMatrix() * mTransform;
    mIsTransformDirty = false;
    return mCachedTransform;
}

void Frame::invalidateTransformRecursively() {
    invalidateTransform(); 
    for (const auto& frame : mChilds) {
        frame->invalidateTransformRecursively();
    }

    updateBoundingVolumes();
}

void Frame::setBBOX(const std::pair<glm::vec3, glm::vec3>& bbox) {
    mAABB = bbox;
    updateBoundingVolumes();
}

void Frame::updateBoundingVolumes() {
    const auto currentWorld = getWorldMatrix();
    const auto min = currentWorld * glm::vec4(mAABB.first, 1.0f);
    const auto max = currentWorld * glm::vec4(mAABB.second, 1.0f);
    mABBBWorld = std::make_pair<glm::vec3, glm::vec3>(min, max);
    mSphereBounding = std::make_unique<Sphere>((mABBBWorld.second + mABBBWorld.first) * 0.5f, glm::length(mABBBWorld.first - mABBBWorld.second));
}

std::shared_ptr<Frame> Frame::findFrame(const std::string& name) const {
    auto res = std::find_if(mChilds.begin(), mChilds.end(), [&name](const auto& a) { return a->getName() == name; });
    if (res != mChilds.end())
        return *res;

    std::shared_ptr<Frame> foundFrame = nullptr;
    for (const auto& frame : mChilds) {
        foundFrame = frame->findFrame(name);
        if (foundFrame != nullptr) 
            return foundFrame;
    }

    return nullptr;
}

void Frame::setPos(const glm::vec3& pos) {
    mPos = pos;
    updateTransform();
}

void Frame::setRot(const glm::quat& rot) {
    mRot = rot;
    updateTransform();
}

void Frame::setScale(const glm::vec3& scale) {
    mScale = scale;
    updateTransform();
}

void Frame::updateTransform() {
    const auto translation = glm::translate(glm::mat4(1.f), mPos);
    const auto scale = glm::scale(glm::mat4(1.f), mScale);
    const auto rot = glm::mat4(1.f) * glm::toMat4(mRot);
    const auto world = translation * rot * scale;
    setMatrix(world);
}