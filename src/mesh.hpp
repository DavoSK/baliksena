#pragma once
#include "frame.hpp"
#include "renderer.hpp"

#include <vector>
#include <memory>

class Mesh;
class Material;

class FaceGroup {
public:
    explicit FaceGroup(std::vector<uint16_t> indices, std::weak_ptr<Mesh> mesh) :
        mIndices(std::move(indices)),
        mMesh(mesh) {
    }

    void setMaterial(std::shared_ptr<Material> mat) { mMaterial = std::move(mat); }
    [[nodiscard]] const std::shared_ptr<Material>& getMaterial() { return mMaterial; }

    const std::vector<uint16_t>& getIndices() { return mIndices; }
    void setOffset(size_t offset) { mOffset = offset; }

    void render() const;
private:
    size_t mOffset = 0;
    std::weak_ptr<Mesh> mMesh;
    std::vector<uint16_t> mIndices;
    std::shared_ptr<Material> mMaterial;
};

class Mesh : public Frame {
public:    
    void setVertices(std::vector<Vertex> vertices) { mVertices = std::move(vertices); }
    [[nodiscard]] const std::vector<Vertex>& getVertices () { return mVertices; }

    void addFaceGroup(std::unique_ptr<FaceGroup> faceGroup) { mFaceGroups.push_back(std::move(faceGroup)); }
    [[nodiscard]] const std::vector<std::unique_ptr<FaceGroup>>& getFaceGroups() { return mFaceGroups; }

    void render() override;
private:
    std::vector<Vertex> mVertices;
    std::vector<std::unique_ptr<FaceGroup>> mFaceGroups;
};