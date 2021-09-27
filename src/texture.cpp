//
// Created by david on 25. 4. 2021.
//

#include "texture.hpp"
#include "renderer.hpp"
#include "bmp_loader.hpp"

#include <filesystem>
#include <fstream>

std::unordered_map<std::string, std::shared_ptr<Texture>> gTextureCache;

void Texture::clearCache() 
{
    gTextureCache.clear();
}

std::weak_ptr<Texture> Texture::loadFromFile(const std::string& fileName, bool useTransparencyKey) 
{
    auto texture = gTextureCache[fileName];
    if (texture == nullptr) 
    {
        auto path = "C:\\Mafia\\MAPS\\" + fileName;
        std::ifstream textureFile(path, std::ifstream::binary);
        if (textureFile.good()) 
        {
            //NOTE: init new texture
            auto newTexture             = std::make_shared<Texture>();
            newTexture->mTextureName    = fileName;
            newTexture->mBuffer         = loadBMP(textureFile, &newTexture->mWidth, &newTexture->mHeight, useTransparencyKey);
            
            if (!newTexture->mBuffer)
                return {};

            newTexture->mTextureHandle = Renderer::createTexture(newTexture->mBuffer, newTexture->mWidth, newTexture->mHeight);
            return gTextureCache[fileName] = std::move(newTexture);
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