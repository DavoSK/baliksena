#include "frame.hpp"

class Sector : public Frame {
public:
    FrameType getType() const override { return FrameType::SECTOR; }
};