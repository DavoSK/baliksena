//
// Created by david on 25. 4. 2021.
//

#include "material.hpp"
#include "renderer.hpp"
#include "texture.hpp"

#include <sokol/sokol_time.h>

void Material::bind() {
    if (isAnimated()) {
        if (stm_sec(stm_diff(stm_now(), mLastUpdatedAnimTex)) > mAnimationPeriod / 1000.0f) {
            mCurrentAnimatedDiffuseIdx++;
            if (mCurrentAnimatedDiffuseIdx >= mAnimatedTextures.size()) mCurrentAnimatedDiffuseIdx = 0;
            mRenderMaterial.diffuseTexture = mAnimatedTextures[mCurrentAnimatedDiffuseIdx]->getTextureHandle();
            mLastUpdatedAnimTex = stm_now();
        }
    }

    Renderer::bindMaterial(mRenderMaterial);
}

void Material::createTextureForSlot(unsigned int slot, const std::string& path) {
    auto* texture = Texture::loadFromFile(path, mHasTransparencyKey);
    switch(static_cast<TextureSlots>(slot)) {
        case TextureSlots::DIFFUSE: {
            mRenderMaterial.diffuseTexture = texture->getTextureHandle();
        } break;

        // case TextureSlots::ALPHA: {
        //     mRenderMaterial.alphaTexture = texture->getTextureHandle();
        // } break;

         case TextureSlots::ENV: {
             mRenderMaterial.envTexture = texture->getTextureHandle();
         } break;

        default: 
            break;
    }
    
    mTextures[slot] = texture;
}

void Material::appendAnimatedTexture(const std::string& path) {
    mAnimatedTextures.push_back(Texture::loadFromFile(path, mHasTransparencyKey));
}

void Material::init() {
    mRenderMaterial.envTextureBlending = this->getTextureBlending();
    mRenderMaterial.envTextureBlendingRatio = this->getEnvRatio();
    mRenderMaterial.isDoubleSided = mIsDoubleSided;
    mRenderMaterial.transparency = mTransparency;
}