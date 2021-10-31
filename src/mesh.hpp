#pragma once
#include "frame.hpp"
#include "renderer.hpp"

#include <vector>
#include <memory>

class Mesh;
class Material;

class FaceGroup {
public:
    explicit FaceGroup(
        std::vector<uint16_t> indices, 
		std::weak_ptr<Mesh> mesh) :
            mIndices(std::move(indices)),
            mMesh(mesh) {
        mIndicesCount = mIndices.size();
    }

    void setMaterial(std::shared_ptr<Material> mat) { mMaterial = std::move(mat); }
    [[nodiscard]] const std::shared_ptr<Material>& getMaterial() { return mMaterial; }

    const std::vector<uint16_t>& getIndices() { return mIndices; }
    void setOffset(size_t offset) { mOffset = offset; }

    void render() const;
private:
    size_t mOffset = 0;
    size_t mIndicesCount = 0;
    std::weak_ptr<Mesh> mMesh;
    std::vector<uint16_t> mIndices;
    std::shared_ptr<Material> mMaterial;
};

class Mesh : public Frame {
public:
    [[nodiscard]] constexpr FrameType getFrameType() const override { return FrameType::Mesh; }

    void setVertices(std::vector<Renderer::Vertex> vertices);
    [[nodiscard]] const std::vector<Renderer::Vertex>& getVertices () { return mVertices; }

    void addFaceGroup(std::unique_ptr<FaceGroup> faceGroup) { mFaceGroups.push_back(std::move(faceGroup)); }
    [[nodiscard]] const std::vector<std::unique_ptr<FaceGroup>>& getFaceGroups() { return mFaceGroups; }

    virtual void render() override;

    void setStatic(bool isStatic) { mStatic = isStatic; }
    [[nodiscard]] bool isStatic() { return mStatic; }
private:
    std::vector<Renderer::Vertex> mVertices;
    std::vector<std::unique_ptr<FaceGroup>> mFaceGroups;
    bool mStatic = true;
};
