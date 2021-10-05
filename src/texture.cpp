//
// Created by david on 25. 4. 2021.
//

#include "texture.hpp"
#include "renderer.hpp"
#include "bmp_loader.hpp"

#include <filesystem>
#include <fstream>
#include <unordered_map>

std::unordered_map<std::string, Texture*> gTextureCache;

void Texture::clearCache() 
{
    for(auto& [textureName, texture] : gTextureCache) {
        if(texture != nullptr) {
            delete texture;
        }
    }

    gTextureCache.clear();
}

Texture* Texture::loadFromFile(const std::string& fileName, bool useTransparencyKey) 
{
    Texture* texture = gTextureCache[fileName];
    if (texture == nullptr) 
    {
        auto path = "C:\\Mafia\\MAPS\\" + fileName;
        std::ifstream textureFile(path, std::ifstream::binary);
        if (textureFile.good()) 
        {
            //NOTE: init new texture
            auto newTexture             = new Texture();
            newTexture->mTextureName    = fileName;
            newTexture->mBuffer         = loadBMP(textureFile, &newTexture->mWidth, &newTexture->mHeight, useTransparencyKey);
            
            if (!newTexture->mBuffer)
                return {};

            newTexture->mTextureHandle = Renderer::createTexture(newTexture->mBuffer, newTexture->mWidth, newTexture->mHeight);
            return gTextureCache[fileName] = newTexture;
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