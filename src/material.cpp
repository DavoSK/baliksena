//
// Created by david on 25. 4. 2021.
//

#include "material.hpp"
#include "renderer.hpp"
#include "texture.hpp"
#include "logger.hpp"

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

void Material::appendAnimatedTexture(const std::string& path) {
    mAnimatedTextures.push_back(Texture::loadFromFile(path, hasTransparencyKey()));
}

void Material::createDiffuseTexture(const std::string& path) {
    if(auto diffuse = Texture::loadFromFile(path, hasTransparencyKey())) {
        mRenderMaterial.diffuseTexture = diffuse->getTextureHandle();
    } else {
        Logger::get().error("unable to create diffsue texture: {} for material", path);
    }
} 

void Material::createEnvTexture(const std::string& path) {
    if(auto env = Texture::loadFromFile(path, hasTransparencyKey())) {
        mRenderMaterial.envTexture = env->getTextureHandle();
    } else {
        Logger::get().error("unable to create env texture: {} for material", path);
    }
}

void Material::createAlphaTexture(const std::string& path) {
    if(auto alpha = Texture::loadFromFile(path, hasTransparencyKey())) {
        mRenderMaterial.alphaTexture = alpha->getTextureHandle();
    } else {
        Logger::get().error("unable to create alpha texture: {} for material", path);
    }
}