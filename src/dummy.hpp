#include "frame.hpp"

class Dummy : public Frame {
public:
	FrameType getType() const override { return FrameType::DUMMY; }
};