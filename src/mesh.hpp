#pragma once
#include "frame.hpp"
#include "renderer.hpp"

#include <vector>
#include <memory>

class Mesh;
class Texture;

class FaceGroup {
public:
    ~FaceGroup();
    explicit FaceGroup(std::vector<uint16_t> indices, Mesh* mesh) :
        mIndices(std::move(indices)),
        mMesh(mesh) {
    }

    void render() const;
    void init();

    void setMaterial(std::shared_ptr<Texture> mat) { mMaterial = std::move(mat); }
    [[nodiscard]] const std::shared_ptr<Texture>& getMaterial() { return mMaterial; }

private:
    Mesh* mMesh = nullptr;
    std::vector<uint16_t> mIndices;
    std::shared_ptr<Texture> mMaterial = nullptr;
    BufferHandle mIndexBuffer{0};
};

class Mesh : public Frame {
public:
    ~Mesh();
    explicit Mesh(std::vector<Vertex> vertices) :
        mVertices(std::move(vertices)) {}

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