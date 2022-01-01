#pragma once
#include "frame.hpp"
#include <glm/glm.hpp>
#include <string>

enum class LightType {
    Dir,
    Point,
    Ambient,
    Spot,
    Fog
};

class Light : public Frame {
public:
    [[nodiscard]] constexpr FrameType getFrameType() const override { return FrameType::Light; }
    
    void setType(LightType type) { mLightType = type; }
    [[nodiscard]] LightType getType() const { return mLightType; }

    void setDir(const glm::vec3& dir) { mDirection = dir; }
    [[nodiscard]] const glm::vec3& getDir() const { return mDirection; }

    void setPos(const glm::vec3& pos) { mPosition = pos; }
    [[nodiscard]] const glm::vec3& getPos() const { return mPosition; }

    void setRange(const glm::vec2& range) { mRange = range; }
    [[nodiscard]] const glm::vec2& getRange() const { return mRange; }

    void setCone(float theta, float phi) { mCone = { theta, phi }; }
    [[nodiscard]] const glm::vec2& getCone() const { return mCone; }

    void setAmbient(const glm::vec3& ambient) { mAmbient = ambient; }
    [[nodiscard]] const glm::vec3& getAmbient() const { return mAmbient; }

    void setDiffuse(const glm::vec3& diffuse) { mDiffuse = diffuse; }
    [[nodiscard]] const glm::vec3& getDiffuse() const { return mDiffuse; }

    void render() override;
private:
    glm::vec2 mRange = {0.0f, 0.0f};
    glm::vec2 mCone = {0.0f, 0.0f};
    glm::vec3 mDirection = {0.0f, 0.0f, 0.0f};
    glm::vec3 mPosition = {0.0f, 0.0f, 0.0f};
    glm::vec3 mAmbient = {0.0f, 0.0f, 0.0f};
    glm::vec3 mDiffuse = {0.0f, 0.0f, 0.0f};
    LightType mLightType;
};