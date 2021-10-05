#pragma once
#include "mesh.hpp"
#include "stats.hpp"

class Billboard : public Mesh {
public:
    Billboard() { gStats.billboardsInUse++; }
    ~Billboard() { gStats.billboardsInUse--; }

    FrameType getType() const override { return FrameType::BILLBOARD; }
    void render() override;
};