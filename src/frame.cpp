#include "frame.hpp"
#include <glm/gtc/matrix_transform.hpp>

#include "app.hpp"
#include "scene.hpp"
#include "camera.hpp"

void Frame::render() {
    if (!mOn) return;
    for (auto& child : mChilds) {
        child->render();
    }
}

void Frame::debugRender() {
    if(!mOn) return;
    for (auto& child : mChilds) {
        child->debugRender();
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
    const auto frameType = getFrameType();
    glm::vec3 AABBmin = mAABB.first;
	glm::vec3 AABBmax = mAABB.second;

    for (auto frame : mChilds) {
        frame->invalidateTransformRecursively();
        
        auto childBbox = frame->getBBOX();
        const auto mat = frame->getMatrix();
        glm::vec3 min = glm::translate(mat, childBbox.first)[3];
        glm::vec3 max = glm::translate(mat, childBbox.second)[3];

        AABBmin = glm::min(AABBmin, min);
        AABBmax = glm::max(AABBmax, max);
    }

    if(frameType == FrameType::Model || 
       frameType == FrameType::Mesh) {
        setBBOX(std::make_pair(AABBmin, AABBmax));
    } else {
        updateBoundingVolumes();   
    }
}

void Frame::setBBOX(const std::pair<glm::vec3, glm::vec3>& bbox) {
    mAABB = bbox;
    updateBoundingVolumes();
}

void Frame::updateBoundingVolumes() {
    const auto mat = getWorldMatrix();
    glm::vec3 min = glm::translate(mat, mAABB.first)[3];
    glm::vec3 max = glm::translate(mat, mAABB.second)[3];

    mABBBWorld = std::make_pair(min, max);
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
    const auto translation = glm::translate(glm::mat4(1.0f), mPos);
    const auto scale = glm::scale(glm::mat4(1.0f), mScale);
    const auto rot = glm::toMat4(mRot);
    const auto world = translation * rot * scale;
    setMatrix(world);
}

void Frame::setMatrix(const glm::mat4& mat) {
    mTransform = mat;
    invalidateTransformRecursively();
}

bool Frame::isVisible() {
    if(Renderer::isCamRelative()) return true;
    if(auto* cam = App::get()->getScene()->getActiveCamera()) {
        if(mSphereBounding != nullptr) {
           return cam->getFrustum().checkSphere(mSphereBounding.get());
        }
    }

    return false;
}