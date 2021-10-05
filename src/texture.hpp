#pragma once
#include <string>
#include "renderer.hpp"

class Texture {
public:
    ~Texture() { printf(" ~Texture()\n"); release(); }
    static void clearCache();
    static Texture* loadFromFile(const std::string& path, bool useTransparencyKey = false);
    void bind(unsigned int slot) const;
    const std::string& getName() const { return mTextureName; }
    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }
    TextureHandle getTextureHandle() const { return mTextureHandle; }
private:
    void release();
    uint8_t* mBuffer{nullptr};
    std::string mTextureName;
    TextureHandle mTextureHandle{0};
    int mWidth = 0;
    int mHeight = 0;
};