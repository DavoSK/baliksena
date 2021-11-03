#pragma once
#include <string>
#include "renderer.hpp"
#include "stats.hpp"

class Texture {
public:
    Texture() { gStats.texturesInUse++; }
    ~Texture() { release(); gStats.texturesInUse--; }
    static void clearCache();
    static Texture* loadFromFile(const std::string& path, bool useTransparencyKey = false, bool mipmaps = false);
    void bind(unsigned int slot) const;
    const std::string& getName() const { return mTextureName; }
    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }
    Renderer::TextureHandle getTextureHandle() const { return mTextureHandle; }
private:
    void release();
    uint8_t* mBuffer{nullptr};
    std::string mTextureName;
    Renderer::TextureHandle mTextureHandle{0};
    int mWidth = 0;
    int mHeight = 0;
};