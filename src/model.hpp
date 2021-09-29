#pragma once
#include "frame.hpp"
#include "renderer.hpp"

class Model : public Frame {
public:
    void render() override;
    void init();
private:
    BufferHandle mVertexBuffer{ 0 };
    BufferHandle mIndexBuffer{ 0 };
};