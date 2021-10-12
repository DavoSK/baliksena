#pragma once
#include "renderer.hpp"
#include "stats.hpp"

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

enum class FrameType
{
    FRAME, 
    MESH,
    DUMMY,
    SECTOR,
    BILLBOARD,
};

class Frame {
public:
    Frame() :
        mTransform(glm::mat4(1.0f)),
        mCachedTransform(mTransform),
        mName("empty"),
        mOn(true),
        mIsTransformDirty(true),
        mOwner(nullptr) {
        gStats.framesInUse++;
    }

    ~Frame() {
        gStats.framesInUse--;
    }

    virtual constexpr FrameType getType() const { return FrameType::FRAME; }
    virtual void render();

    void setOwner(Frame* frame) { mOwner = frame; }
    Frame* getOwner() { return mOwner; }

    void setName(const std::string& name) { mName = name;  }
    const std::string& getName() const { return mName;  }

    void addChild(std::shared_ptr<Frame> frame);
    const std::vector<std::shared_ptr<Frame>>& getChilds() { return mChilds; };
    void clear() { mChilds.clear(); }
    
    const glm::mat4& getWorldMatrix();
    const glm::mat4& getMatrix() { return mTransform; }
    void setMatrix(const glm::mat4& mat) {
        mTransform = mat;
        invalidateTransformRecursively();
    }

    const std::pair<glm::vec3, glm::vec3>& getWorldBBOX() const { return mABBBWorld; }

    void setBBOX(const std::pair<glm::vec3, glm::vec3>& bbox);
    const std::pair<glm::vec3, glm::vec3>& getBBOX() const { return mAABB; }

    void invalidateTransform() { mIsTransformDirty = true; }
    void invalidateTransformRecursively();

    void setOn(bool on) { mOn = on; }
    bool isOn() const { return mOn; }

    Frame* findNode(const std::string& name) const;
    Frame* findNodeMaf(const std::string& path) const ;
private:
    void updateAABBWorld();
    std::pair<glm::vec3, glm::vec3> mAABB;
    std::pair<glm::vec3, glm::vec3> mABBBWorld;

    bool mIsTransformDirty;
    bool mOn;
    std::string mName;
    glm::mat4 mTransform;
    glm::mat4 mCachedTransform;
    Frame* mOwner;
    std::vector<std::shared_ptr<Frame>> mChilds;
};