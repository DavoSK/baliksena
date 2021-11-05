#pragma once
#include "frame.hpp"
#include <glm/glm.hpp>
#include <string>

enum class LightType {
    Dir,
    Point,
    Ambient,
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

    void setFar(float newFar) { mFar = newFar; }
    [[nodiscard]] float getFar() const { return mFar; }

    void setNear(float newNear) { mNear = newNear; }
    [[nodiscard]] float getNear() const { return mNear; }

    void setAmbient(const glm::vec3& ambient) { mAmbient = ambient; }
    [[nodiscard]] const glm::vec3& getAmbient() const { return mAmbient; }

    void setDiffuse(const glm::vec3& diffuse) { mDiffuse = diffuse; }
    [[nodiscard]] const glm::vec3& getDiffuse() const { return mDiffuse; }

    void setSpecular(const glm::vec3& spec) { mSpecular = spec; }
    [[nodiscard]] const glm::vec3& getSpecular() const { return mSpecular; }

    void render() override;
private:
    float mFar = 0.0f;
    float mNear = 0.0f;
    glm::vec3 mDirection = {0.0f, 0.0f, 0.0f};
    glm::vec3 mPosition = {0.0f, 0.0f, 0.0f};
    glm::vec3 mAmbient = {0.0f, 0.0f, 0.0f};
    glm::vec3 mDiffuse = {0.0f, 0.0f, 0.0f};
    glm::vec3 mSpecular = {0.0f, 0.0f, 0.0f};
    LightType mLightType;
};