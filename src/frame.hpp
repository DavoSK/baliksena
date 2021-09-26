#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

class Frame {
public:
    Frame(const glm::mat4& transform, const std::string& frameName) :
        mTransform(transform),
        mCachedTransform(transform),
        mName(frameName),
        mOn(true),
        mIsTransformDirty(true),
        mOwner(nullptr) {
    }

    Frame(const glm::mat4& transform = glm::mat4(1.0f)) :
        mTransform(transform),
        mCachedTransform(transform),
        mName("empty"),
        mOn(true),
        mIsTransformDirty(true),
        mOwner(nullptr) {
    }

    virtual void render(const glm::mat4& mat);

    void setOwner(Frame* frame) { mOwner = frame; }
    Frame* getOwner() { return mOwner; }

    void setName(const std::string& name) { mName = name;  }
    const std::string& getName() const { return mName;  }

    void addChild(std::shared_ptr<Frame> frame);
    const std::vector<std::shared_ptr<Frame>>& getChilds() { return mChilds; };

    const glm::mat4& getWorldMatrix();
    const glm::mat4& getMatrix() { return mTransform; }
    void setMatrix(const glm::mat4& mat) {
        mTransform = mat;
        invalidateTransformRecursively();
    }

    void invalidateTransform() { mIsTransformDirty = true; }
    void invalidateTransformRecursively();
private:
    bool mIsTransformDirty;
    bool mOn;
    std::string mName;
    glm::mat4 mTransform;
    glm::mat4 mCachedTransform;
    Frame* mOwner;
    std::vector<std::shared_ptr<Frame>> mChilds;
};