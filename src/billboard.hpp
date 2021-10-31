#pragma once
#include "mesh.hpp"
#include "stats.hpp"

class Billboard : public Mesh {
public:
    Billboard() { gStats.billboardsInUse++; }
    ~Billboard() { gStats.billboardsInUse--; }

    [[nodiscard]] constexpr FrameType getFrameType() const override { return FrameType::Billboard; }
    void render() override;
};