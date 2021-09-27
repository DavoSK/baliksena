//
// Created by david on 25. 4. 2021.
//

#include "material.hpp"
#include "renderer.hpp"
#include "texture.hpp"

void Material::bind() {
    // NOTE: update animated texture
    /*if (isAnimated()) {
        const auto currentTime = Window::get()->getTime();
        if (currentTime - mLastUpdated > mAnimationPeriod / 1000.0f) {
            mCurrentAnimatedDiffuseIdx++;
            if (mCurrentAnimatedDiffuseIdx >= mAnimatedTextures.size()) mCurrentAnimatedDiffuseIdx = 0;
            mLastUpdated = currentTime;
        }        mAnimatedTextures.push_back(texture);
    }*/

    Renderer::bindMaterial(mRenderMaterial);
}

void Material::createTextureForSlot(unsigned int slot, const std::string& path) {
    std::weak_ptr<Texture> texturePtr = Texture::loadFromFile(path, mHasTransparencyKey);
        if(auto texture = texturePtr.lock()) {
        
        switch(static_cast<TextureSlots>(slot)) {
            case TextureSlots::DIFFUSE: {
                mRenderMaterial.diffuseTexture = texture->getTextureHandle();
            } break;

            // case TextureSlots::ALPHA: {
            //     mRenderMaterial.alphaTexture = texture->getTextureHandle();
            // } break;

            // case TextureSlots::ENV: {
            //     mRenderMaterial.envTexture = texture->getTextureHandle();
            // } break;

            default: 
                break;
        }
    }

    mTextures[slot] = texturePtr;
}

void Material::appendAnimatedTexture(const std::string& path) {
    mAnimatedTextures.push_back(Texture::loadFromFile(path, mHasTransparencyKey));
}