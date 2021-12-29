#pragma once
#include "frame.hpp"
#include "renderer.hpp"
#include "stats.hpp"

#include <unordered_map>
#include <memory>

struct RenderHelper {
    size_t vertexBufferOffset;
    size_t verticesCount;
};

class Material;
class Model : public Frame {
public:
    Model() { gStats.modelsInUse++; }
    ~Model() { gStats.modelsInUse--; }
    constexpr FrameType getFrameType() const override { return FrameType::Model; }

    void render() override;
    void debugRender() override;
    void init();
protected:
    Material* mMat = nullptr;
    std::vector<Renderer::Vertex> mVertices;
    std::vector<uint32_t> mIndices;
};