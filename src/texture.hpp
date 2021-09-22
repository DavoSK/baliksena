#pragma once
#include <unordered_map>
#include <memory>
#include <string>

#include "renderer.hpp"

class Texture {
public:
    ~Texture() { release(); }
    static std::shared_ptr<Texture> loadFromFile(const std::string& path, bool useTransparencyKey = false);
    void bind(unsigned int slot) const;
    [[nodiscard]] const std::string& getName() const { return mTextureName; }
    [[nodiscard]] int getWidth() const { return mWidth; }
    [[nodiscard]] int getHeight() const { return mHeight; }
    [[nodiscard]] TextureHandle getTextureHandle() const { return mTextureHandle; }
private:
    void release();
    uint8_t* mBuffer{nullptr };
    std::string mTextureName;
    TextureHandle mTextureHandle;
    int mWidth = 0;
    int mHeight = 0;
};