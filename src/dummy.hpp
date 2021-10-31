#include "frame.hpp"

class Dummy : public Frame {
public:
	[[nodiscard]] constexpr FrameType getFrameType() const override { return FrameType::Dummy; }
};