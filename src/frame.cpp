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

    updateAABBWorld();
}

void Frame::setBBOX(const std::pair<glm::vec3, glm::vec3>& bbox) {
    mAABB = bbox;
    updateAABBWorld();
}

void Frame::updateAABBWorld() {
    const auto currentWorld = getWorldMatrix();
    // const auto min = glm::translate(currentWorld, mAABB.first);
    // const auto max = glm::translate(currentWorld, mAABB.second);
    // mABBBWorld = std::make_pair<glm::vec3, glm::vec3>(min[3], max[3]);
    const auto min = currentWorld * glm::vec4(mAABB.first, 1.0f);
    const auto max = currentWorld * glm::vec4(mAABB.second, 1.0f);
    mABBBWorld = std::make_pair<glm::vec3, glm::vec3>(min, max);
}

std::vector<std::string> split(std::string const& original, char separator) {
    std::vector<std::string> results;
    std::string::const_iterator start = original.begin();
    std::string::const_iterator end = original.end();
    std::string::const_iterator next = std::find(start, end, separator);
    while (next != end) {
        results.push_back(std::string(start, next));
        start = next + 1;
        next = std::find(start, end, separator);
    }
    results.push_back(std::string(start, next));
    return results;
}

std::shared_ptr<Frame> Frame::findNode(const std::string& name) const {
    auto res = std::find_if(mChilds.begin(), mChilds.end(), [&name](const auto& a) { return a->getName() == name; });
    if (res != mChilds.end())
        return *res;

    std::shared_ptr<Frame> foundNode = nullptr;
    for (const auto& node : mChilds) {
        foundNode = node->findNode(name);
        if (foundNode != nullptr) 
            return foundNode;
    }

    return nullptr;
}

std::shared_ptr<Frame> Frame::findNodeMaf(const std::string& path) const {
    if(path.find(".") != std::string::npos) {
        auto splitedPath = split(path, '.');
        auto parentNode = findNode(splitedPath[0]);
        return parentNode != nullptr ? parentNode->findNode(splitedPath[1]) : nullptr;
        /*if(!parentNode) {
            return nullptr;
        }
        
        for(size_t i = 1; i < splitedPath.size() - 1; i++) {
            parentNode = parentNode->findNode(splitedPath[i]);
            if(parentNode == nullptr) {
                return nullptr;
            }
        }

        return parentNode;*/
    }

    return findNode(path);
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