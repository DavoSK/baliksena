#pragma once
#include "frame.hpp"
#include "renderer.hpp"
#include <unordered_map>


class Material;

struct RenderHelper {
    size_t vertexBufferOffset;
    size_t verticesCount;
};

class Model : public Frame {
public:
    ~Model() { printf("~Model()\n"); }
    void render() override;
    void init();
private:
    Material* mMat = nullptr;
    std::unordered_map<Material*, RenderHelper> mRenderHelper;
    std::vector<Vertex> mVertices;
    std::vector<uint32_t> mIndices;
    BufferHandle mVertexBuffer{ 0 };
    BufferHandle mIndexBuffer{ 0 };
};