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

    //Renderer::bindMaterial(this);
}

void Material::createTextureForSlot(unsigned int slot, const std::string& path) {
    mTextures[slot] = Texture::loadFromFile(path, mHasTransparencyKey);
}

void Material::appendAnimatedTexture(const std::string& path) {
    auto texture = Texture::loadFromFile(path, mHasTransparencyKey);
    if (texture) {
    }
}