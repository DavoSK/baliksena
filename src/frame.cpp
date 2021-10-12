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
    const auto min = glm::translate(currentWorld, mAABB.first);
    const auto max = glm::translate(currentWorld, mAABB.second);
    mABBBWorld = std::make_pair<glm::vec3, glm::vec3>(min[3], max[3]);
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

Frame* Frame::findNode(const std::string& name) const {
    auto res = std::find_if(mChilds.begin(), mChilds.end(), [&name](const auto& a) { return a->getName().compare(name) == 0; });
    if (res != mChilds.end())
        return res->get();

    Frame* foundNode = nullptr;
    for (const auto& node : mChilds) {
        foundNode = node->findNode(name);
        if (foundNode != nullptr) 
            return foundNode;
    }

    return nullptr;
}

Frame* Frame::findNodeMaf(const std::string& path) const {
    if(path.find(".") != std::string::npos) {
        auto splitedPath = split(path, '.');
        auto parentNode = findNode(splitedPath[0]);
        return parentNode != nullptr ? parentNode->findNode(splitedPath[1]) : nullptr;
    }

    return findNode(path);
}
