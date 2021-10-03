#pragma once
#include "mesh.hpp"

class Billboard : public Mesh {
public:
    FrameType getType() const override { return FrameType::BILLBOARD; }
    void render() override;
};