#pragma once
#include "frame.hpp"
#include "renderer.hpp"

#include <vector>
#include <memory>

class Mesh;
class Material;

class FaceGroup {
public:
    ~FaceGroup();
    explicit FaceGroup(std::vector<uint16_t> indices, std::weak_ptr<Mesh> mesh) :
        mIndices(std::move(indices)),
        mMesh(mesh) {
    }

    void render() const;
    void init();

    void setMaterial(std::unique_ptr<Material> mat) { mMaterial = std::move(mat); }
    [[nodiscard]] const std::unique_ptr<Material>& getMaterial() { return mMaterial; }

private:
    std::weak_ptr<Mesh> mMesh;
    std::vector<uint16_t> mIndices;
    std::unique_ptr<Material> mMaterial;
    BufferHandle mIndexBuffer{0};
};

class Mesh : public Frame {
public:
    ~Mesh();
    
    void setVertices(std::vector<Vertex> vertices) {
        mVertices = std::move(vertices);
    }

    void addFaceGroup(std::unique_ptr<FaceGroup> faceGroup) {
        mFaceGroups.push_back(std::move(faceGroup));
    }

    [[nodiscard]] const std::vector<std::unique_ptr<FaceGroup>>& getFaceGroups() { return mFaceGroups; }

    void render() override;
    void init();
private:
    std::vector<Vertex> mVertices;
    std::vector<std::unique_ptr<FaceGroup>> mFaceGroups;
    BufferHandle mVertexBuffer{ 0 };
};