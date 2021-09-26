//
// Created by david on 25. 4. 2021.
//

#include "texture.hpp"
#include "renderer.hpp"
#include "bmp_loader.hpp"

#include <filesystem>
#include <fstream>

std::shared_ptr<Texture> Texture::loadFromFile(const std::string& path, bool useTransparencyKey) {
    auto texture = TextureCache::get(path);
    if (!texture) {
        std::ifstream textureFile(path.c_str(), std::ifstream::binary);
        if (textureFile.good()) {
            auto newTexture = std::make_shared<Texture>();
            newTexture->mTextureName = std::filesystem::path(path.c_str()).filename().string().c_str();
            newTexture->mBuffer = loadBMP(textureFile, &newTexture->mWidth, &newTexture->mHeight, useTransparencyKey);
            
            if (!newTexture->mBuffer)
                return nullptr;

            newTexture->mTextureHandle = Renderer::createTexture(newTexture->mBuffer, newTexture->mWidth, newTexture->mHeight);
            return TextureCache::add(std::move(newTexture), path);
        }
    }

    return texture;
}

void Texture::bind(unsigned int slot) const {
    Renderer::bindTexture(mTextureHandle, slot);
}

void Texture::release() {
    Renderer::destroyTexture(mTextureHandle);
    if (mBuffer != nullptr) {
        delete mBuffer;
        mBuffer = nullptr;
    }
}

std::unordered_map<std::string, std::shared_ptr<Texture>> TextureCache::mCache;