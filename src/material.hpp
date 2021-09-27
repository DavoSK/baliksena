//
// Created by david on 27. 9. 2021.
//

#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <array>
#include <vector>
#include <string>

#include "renderer.hpp"

enum TextureSlots { DIFFUSE = 0, ALPHA, ENV, ANIMATED, USER_END };

class Texture;
class Material {
public:
    ~Material() {
        printf("~Material()\n");
    }

    void bind();
    void createTextureForSlot(unsigned int slot, const std::string& path);
    void appendAnimatedTexture(const std::string& path);

    [[nodiscard]] bool hasTexture(size_t textureIndex) const {
        return (textureIndex < TextureSlots::USER_END - 1 && !mTextures[textureIndex].expired());
    }

    [[nodiscard]] std::weak_ptr<Texture> getTexture(size_t textureIndex) { return mTextures[textureIndex]; }
    [[nodiscard]] std::weak_ptr<Texture> getAnimatedTexture(size_t textureIndex) { return mAnimatedTextures[textureIndex]; }
    [[nodiscard]] std::weak_ptr<Texture> getCurrentAnimatedDiffuse() { return mAnimatedTextures[mCurrentAnimatedDiffuseIdx]; }

    void setAmbient(const glm::vec3& color) { mAmbient = color; }
    [[nodiscard]] const glm::vec3& getAmbient() const { return mAmbient; }

    void setDiffuse(const glm::vec3& color) { mDiffuse = color; }
    [[nodiscard]] const glm::vec3& getDiffuse() const { return mDiffuse; }

    void setEmission(const glm::vec3& color) { mEmission = color; }

    void setAditiveMixing(bool mixing) { mAditiveMixing = mixing; }
    [[nodiscard]] bool hasAditiveMixing() const { return mAditiveMixing; }

    void setEnvRatio(float val) { mEnvRatio = val; }
    [[nodiscard]] float getEnvRatio() const { return mEnvRatio; }

    void setTransparency(const float val) { mTransparency = val; }
    [[nodiscard]] float getTransparency() const { return mTransparency; }

    bool isTransparent() const { return mTransparency < 1.0f || hasTexture(TextureSlots::ALPHA); }

    void setDoubleSided(bool val) { mIsDoubleSided = val; }
    [[nodiscard]] bool isDoubleSided() const { return mIsDoubleSided; }

    void setColored(bool val) { mIsColored = val; }

    void setTextureBlending(TextureBlending blending) { mBlending = blending; }
    [[nodiscard]] TextureBlending getTextureBlending() const { return mBlending; }

    void setHasTransparencyKey(bool hasKey) { mHasTransparencyKey = hasKey; }
    [[nodiscard]] bool hasTransparencyKey() const { return mHasTransparencyKey; }

    void setAnimated(bool animated, uint32_t framePeriod) {
        mIsAnimated = animated;
        mAnimationPeriod = framePeriod;
    }

    [[nodiscard]] bool isAnimated() const { return mIsAnimated; }
private:
    RendererMaterial mRenderMaterial{};
    std::array<std::weak_ptr<Texture>, TextureSlots::USER_END> mTextures;
    std::vector<std::weak_ptr<Texture>> mAnimatedTextures;
    glm::vec3 mAmbient                  = {0.0f, 0.0f, 0.0f};
    glm::vec3 mDiffuse                  = {0.0f, 0.0f, 0.0f};
    glm::vec3 mEmission                 = {0.0f, 0.0f, 0.0f};
    float mEnvRatio                     = 0.0f;
    float mTransparency                 = 1.0f;
    uint32_t mAnimationPeriod           = 0;
    size_t mCurrentAnimatedDiffuseIdx   = 0;
    double mLastUpdated                 = 0.0f;
    bool mAditiveMixing                 = false;
    bool mIsAnimated                    = false;
    bool mIsDoubleSided                 = false;
    bool mIsColored                     = false;
    bool mHasTransparencyKey            = false;
    TextureBlending mBlending           = TextureBlending::NORMAL;
};