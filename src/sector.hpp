#include "frame.hpp"
#include <vector>

class Light;
class Sound;
class Sector : public Frame {
public:
    [[nodiscard]] constexpr FrameType getFrameType() const override { return FrameType::Sector; }
    void render() override;

    void pushLight(std::shared_ptr<Light> light);
    [[nodiscard]] const std::vector<std::shared_ptr<Light>>& getLights() { return mSectorLights; }

    void pushSound(std::shared_ptr<Sound> sound);
    [[nodiscard]] const std::vector<std::shared_ptr<Sound>>& getSounds() { return mSectorSounds; }
private:
    std::vector<std::shared_ptr<Light>> mSectorLights;
    std::vector<std::shared_ptr<Sound>> mSectorSounds;
};