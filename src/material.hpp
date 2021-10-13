//
// Created by david on 27. 9. 2021.
//

#pragma once
#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <string>

#include "renderer.hpp"

enum TextureSlots { DIFFUSE = 0, ALPHA, ENV, ANIMATED, USER_END };

class Texture;
class Material {
public:
    Material() {
    }

    ~Material() { 
    }

    void bind();
    void createTextureForSlot(unsigned int slot, const std::string& path);
    void appendAnimatedTexture(const std::string& path);

    bool hasTexture(size_t textureIndex) const {
        return (textureIndex < TextureSlots::USER_END - 1 && mTextures[textureIndex] != nullptr);
    }

    Texture* getTexture(size_t textureIndex) { return mTextures[textureIndex]; }
    Texture* getAnimatedTexture(size_t textureIndex) { return mAnimatedTextures[textureIndex]; }
    Texture* getCurrentAnimatedDiffuse() { return mAnimatedTextures[mCurrentAnimatedDiffuseIdx]; }

    void setAmbient(const glm::vec3& color) { mAmbient = color; }
    const glm::vec3& getAmbient() const { return mAmbient; }

    void setDiffuse(const glm::vec3& color) { mDiffuse = color; }
    const glm::vec3& getDiffuse() const { return mDiffuse; }

    void setEmission(const glm::vec3& color) { mEmission = color; }

    void setAditiveMixing(bool mixing) { mAditiveMixing = mixing; }
    bool hasAditiveMixing() const { return mAditiveMixing; }

    void setEnvRatio(float val) { mEnvRatio = val; }
    float getEnvRatio() const { return mEnvRatio; }

    void setTransparency(const float val) { mTransparency = val; }
    float getTransparency() const { return mTransparency; }

    bool isTransparent() const { return mTransparency < 1.0f || hasTexture(TextureSlots::ALPHA); }

    void setDoubleSided(bool val) { mIsDoubleSided = val; }
    bool isDoubleSided() const { return mIsDoubleSided; }

    void setColored(bool val) { mIsColored = val; }

    void setTextureBlending(TextureBlending blending) { mBlending = blending; }
    TextureBlending getTextureBlending() const { return mBlending; }

    void setHasTransparencyKey(bool hasKey) { mHasTransparencyKey = hasKey; }
    bool hasTransparencyKey() const { return mHasTransparencyKey; }

    void setAnimated(bool animated, uint32_t framePeriod) {
        mIsAnimated = animated;
        mAnimationPeriod = framePeriod;
    }

    bool isAnimated() const { return mIsAnimated; }
    
    void setKind(MaterialKind kind) { mRenderMaterial.kind = kind; }
    MaterialKind getKind() const { return mRenderMaterial.kind; }
private:
    RendererMaterial mRenderMaterial{};
    std::array<Texture*, TextureSlots::USER_END> mTextures{nullptr, nullptr, nullptr, nullptr};
    std::vector<Texture*> mAnimatedTextures;
    glm::vec3 mAmbient                  = {0.0f, 0.0f, 0.0f};
    glm::vec3 mDiffuse                  = {0.0f, 0.0f, 0.0f};
    glm::vec3 mEmission                 = {0.0f, 0.0f, 0.0f};
    float mEnvRatio                     = 0.0f;
    float mTransparency                 = 1.0f;
    uint32_t mAnimationPeriod           = 0;
    size_t mCurrentAnimatedDiffuseIdx   = 0;
    uint64_t mLastUpdatedAnimTex        = 0;
    bool mAditiveMixing                 = false;
    bool mIsAnimated                    = false;
    bool mIsDoubleSided                 = false;
    bool mIsColored                     = false;
    bool mHasTransparencyKey            = false;
    TextureBlending mBlending           = TextureBlending::NORMAL;
};