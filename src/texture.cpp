//
// Created by david on 25. 4. 2021.
//

#include "texture.hpp"
#include "renderer.hpp"
#include "bmp_loader.hpp"

#include <filesystem>
#include <fstream>
#include <stdint.h>

std::shared_ptr<Texture> Texture::loadFromFile(const std::string& path, bool useTransparencyKey) {
    std::ifstream textureFile(path.c_str(), std::ifstream::binary);
    if (textureFile.good()) {
        auto newTexture = std::make_shared<Texture>();
        newTexture->mTextureName = std::filesystem::path(path.c_str()).filename().string().c_str();
        newTexture->mBuffer = loadBMP(textureFile, &newTexture->mWidth, &newTexture->mHeight, useTransparencyKey);

        if (!newTexture->mBuffer)
            return nullptr;

        newTexture->mTextureHandle = Renderer::createTexture(newTexture->mBuffer, newTexture->mWidth, newTexture->mHeight);
        return std::move(newTexture);
    }

    return nullptr;
}

void Texture::bind(unsigned int slot) const {
    Renderer::bindTexture(mTextureHandle, slot);
}

void Texture::release() {
    Renderer::destroyTexture(mTextureHandle);
    if (mBuffer != nullptr) {
        delete mBuffer;
    }
}