//
// Created by david on 27. 9. 2021.
//

#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>

#include "renderer.hpp"

class Texture;
class Material {
public:
    void bind();
    
    void createDiffuseTexture(const std::string& path);
    [[nodiscard]] std::optional<Renderer::TextureHandle> getDiffuseTexture() const { return mRenderMaterial.diffuseTexture; }

    void createEnvTexture(const std::string& path);
    [[nodiscard]] std::optional<Renderer::TextureHandle> getEnvTexture() const { return mRenderMaterial.envTexture; }
     
    void createAlphaTexture(const std::string& path);
    [[nodiscard]] std::optional<Renderer::TextureHandle> getAlphaTexture() const { return mRenderMaterial.alphaTexture; }

    void appendAnimatedTexture(const std::string& path);
    
    void setAmbient(const glm::vec3& color) { mRenderMaterial.ambient = color; }
    [[nodiscard]] const glm::vec3& getAmbient() const { return mRenderMaterial.ambient; }

    void setDiffuse(const glm::vec3& color) { mRenderMaterial.diffuse = color; }
    [[nodiscard]] const glm::vec3& getDiffuse() const { return mRenderMaterial.diffuse; }

    void setEmission(const glm::vec3& color) { mRenderMaterial.emission = color; }
    [[nodiscard]] const glm::vec3& getEmission() { return mRenderMaterial.emission; }

    void setEnvRatio(float val) { mRenderMaterial.envTextureBlendingRatio = val; }
    [[nodiscard]] float getEnvRatio() const { return mRenderMaterial.envTextureBlendingRatio; }

    void setTransparency(const float val) { mRenderMaterial.transparency = val; }
    [[nodiscard]] float getTransparency() const { return mRenderMaterial.transparency; }

    void setDoubleSided(bool val) { mRenderMaterial.isDoubleSided = val; }
    [[nodiscard]] bool isDoubleSided() const { return mRenderMaterial.isDoubleSided; }

    void setTextureBlending(Renderer::TextureBlending blending) { mRenderMaterial.envTextureBlending = blending; }
    [[nodiscard]] Renderer::TextureBlending getTextureBlending() const { return mRenderMaterial.envTextureBlending; }

    void setHasTransparencyKey(bool hasKey) { mRenderMaterial.hasTransparencyKey = hasKey; }
    [[nodiscard]] bool hasTransparencyKey() const { return mRenderMaterial.hasTransparencyKey; }

    void setKind(Renderer::MaterialKind kind) { mRenderMaterial.kind = kind; }
    [[nodiscard]] Renderer::MaterialKind getKind() const { return mRenderMaterial.kind; }

    void setColored(bool val) { mRenderMaterial.isColored = val; }
    [[nodiscard]] bool isColored() const { return mRenderMaterial.isColored; }

    void setAnimationPeriod(uint32_t framePeriod) { mAnimationPeriod = framePeriod; }
    [[nodiscard]] bool isAnimated() const { return mAnimationPeriod > 0; }
    
    void setMipmaps(bool mipmaps) { mHasMipmaps = mipmaps; }
    [[nodiscard]] bool hasMipmaps() const { return mHasMipmaps; }

    [[nodiscard]] bool isTransparent() const { return mRenderMaterial.transparency < 1.0f || mRenderMaterial.alphaTexture.has_value(); }
private:
    Renderer::Material mRenderMaterial{};
    std::vector<Texture*> mAnimatedTextures;
    uint32_t mAnimationPeriod           = 0;
    size_t mCurrentAnimatedDiffuseIdx   = 0;
    uint64_t mLastUpdatedAnimTex        = 0;
    bool mHasMipmaps = false;
};