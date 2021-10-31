#include "frame.hpp"
#include <vector>

class Light;
class Sector : public Frame {
public:
    [[nodiscard]] constexpr FrameType getFrameType() const override { return FrameType::Sector; }
    void render() override;
    void pushLight(std::shared_ptr<Light> light) { mSectorLights.push_back(light); }
private:
    void renderLights();
    std::vector<std::shared_ptr<Light>> mSectorLights;
};