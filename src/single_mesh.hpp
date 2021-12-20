#pragma once
#include "frame.hpp"
#include "mesh.hpp"

class Joint : public Frame {
public:
    constexpr FrameType getFrameType() const override { return FrameType::Joint; }

    void setBoneId(uint32_t boneId) { mBoneId = boneId; }
    [[nodiscard]] uint32_t getBoneId() const { return mBoneId; }

    void setLocalMatrix(const glm::mat4& mat) { mLocalJointMatrix = mat; }
    const glm::mat4& getLocalMatrix() const { return mLocalJointMatrix; }
private:
    glm::mat4 mLocalJointMatrix;
    uint32_t mBoneId;
};

struct Bone {
    glm::mat4 mInverseTransform;
    uint32_t mNoneWeightedVertCount;    // amount of vertices that should have a weight of 1.0f
    uint32_t mWeightedVertCount;        // amount of vertices whose weights are stored in mWeights
    uint32_t mBoneID;                   // this is likely a reference to a paired bone, which takes the remainder
                                        // (1.0f - w) of weight
    glm::vec3 mMinBox;
    glm::vec3 mMaxBox;
    std::vector<float> mWeights;
};

class Animator;
class SingleMesh : public Mesh {
public:
    SingleMesh();    
    [[nodiscard]] constexpr FrameType getFrameType() const override { return FrameType::SingleMesh; }

    void setBones(std::vector<Bone> bones) { mBones = std::move(bones); }
    [[nodiscard]] const std::vector<Bone>& getBones() const { return mBones; }

    virtual void render() override;
private:
    std::shared_ptr<Animator> mAnimator{ nullptr };
    std::vector<Bone> mBones;
};
