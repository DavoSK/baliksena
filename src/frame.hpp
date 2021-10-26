#pragma once
#include "renderer.hpp"
#include "stats.hpp"

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtx/quaternion.hpp>

#include <string>
#include <vector>
#include <memory>

enum class FrameType
{
    ALL = -1,
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
    void removeChild(std::shared_ptr<Frame> frame);
    const std::vector<std::shared_ptr<Frame>>& getChilds() { return mChilds; };

    void removeChilds() { mChilds.clear(); }
    
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

    std::shared_ptr<Frame> findNode(const std::string& name) const;
    std::shared_ptr<Frame> findNodeMaf(const std::string& path) const;

    void setPos(const glm::vec3& pos);
    [[nodiscard]] const glm::vec3& getPos() const { return mPos; }

    void setRot(const glm::quat& rot);
    [[nodiscard]] const glm::quat& getRot() const { return mRot; }

    void setScale(const glm::vec3& scale);
    [[nodiscard]] const glm::vec3& getScale() const { return mScale; }
private:
    void updateTransform();
    void updateAABBWorld();
    std::pair<glm::vec3, glm::vec3> mAABB;
    std::pair<glm::vec3, glm::vec3> mABBBWorld;

    bool mIsTransformDirty;
    bool mOn;
    std::string mName;
    glm::vec3 mScale;
    glm::vec3 mPos;
    glm::quat mRot;
    glm::mat4 mTransform;
    glm::mat4 mCachedTransform;
    Frame* mOwner;
    std::vector<std::shared_ptr<Frame>> mChilds;
};