//
// Created by david on 25. 4. 2021.
//

#include "texture.hpp"
#include "renderer.hpp"
#include "bmp_loader.hpp"
#include "vfs.hpp"

#include <filesystem>
#include <fstream>
#include <unordered_map>

std::unordered_map<std::string, Texture*> gTextureCache;

void Texture::clearCache() {
    for(auto& [textureName, texture] : gTextureCache) {
        if(texture != nullptr) {
            delete texture;
        }
    }

    gTextureCache.clear();
}

#include <Windows.h>
// lpBits stand for long pointer bits

// szPathName : Specifies the pathname        -> the file path to save the image
// lpBits    : Specifies the bitmap bits      -> the buffer (content of the) image
// w    : Specifies the image width
// h    : Specifies the image height
bool SaveImage(const char* szPathName, void* lpBits, int w, int h) {

    std::vector<uint8_t> rgbData;

    for(size_t i = 0; i < w * h; i++) {
        RGBQUAD quat = *(RGBQUAD*)( (char*)lpBits + sizeof(RGBQUAD) * i);
        rgbData.push_back(quat.rgbRed);
        rgbData.push_back(quat.rgbGreen);
        rgbData.push_back(quat.rgbBlue);
    }

    // Create a new file for writing
    FILE* pFile = fopen(szPathName, "wb"); // wb -> w: writable b: binary, open as writable and binary
    if (pFile == NULL) {
        return false;
    }

    BITMAPINFOHEADER BMIH;                         // BMP header
    BMIH.biSize = sizeof(BITMAPINFOHEADER);
    BMIH.biSizeImage = w * h * 3;
    // Create the bitmap for this OpenGL context
    BMIH.biSize = sizeof(BITMAPINFOHEADER);
    BMIH.biWidth = w;
    BMIH.biHeight = h;
    BMIH.biPlanes = 1;
    BMIH.biBitCount = 24;
    BMIH.biCompression = BI_RGB;
    BMIH.biSizeImage = w * h * 3;

    BITMAPFILEHEADER bmfh;                         // Other BMP header
    int nBitsOffset = sizeof(BITMAPFILEHEADER) + BMIH.biSize;
    LONG lImageSize = BMIH.biSizeImage;
    LONG lFileSize = nBitsOffset + lImageSize;
    bmfh.bfType = 'B' + ('M' << 8);
    bmfh.bfOffBits = nBitsOffset;
    bmfh.bfSize = lFileSize;
    bmfh.bfReserved1 = bmfh.bfReserved2 = 0;

    // Write the bitmap file header               // Saving the first header to file
    UINT nWrittenFileHeaderSize = fwrite(&bmfh, 1, sizeof(BITMAPFILEHEADER), pFile);

    // And then the bitmap info header            // Saving the second header to file
    UINT nWrittenInfoHeaderSize = fwrite(&BMIH, 1, sizeof(BITMAPINFOHEADER), pFile);

    // Finally, write the image data itself
    //-- the data represents our drawing          // Saving the file content in lpBits to file
    UINT nWrittenDIBDataSize = fwrite(rgbData.data(), 1, lImageSize, pFile);
    fclose(pFile); // closing the file.

    return true;
}

Texture* Texture::loadFromFile(const std::string& fileName, bool useTransparencyKey) {
    auto* texture = gTextureCache[fileName];
    if (texture == nullptr) {
        auto path = "MAPS\\" + fileName;   
        auto savePath = "SAVE\\" + path;

        if(fileName.find("5OKNA08") != std::string::npos) {
            int test = 0;
            test++;
        }

        auto textureFile = Vfs::getFile(path);
        if (textureFile.size()) {
            //NOTE: init new texture
            auto newTexture             = new Texture();
            newTexture->mTextureName    = fileName;
            newTexture->mBuffer         = loadBMP(textureFile, &newTexture->mWidth, &newTexture->mHeight, useTransparencyKey);
          
            if (!newTexture->mBuffer)
                return {};

            // if(!SaveImage(savePath.c_str(), newTexture->mBuffer, newTexture->mWidth, newTexture->mHeight)) {
            //     int test = 0;
            //     test++;
            // }
            if(fileName.find("5OKNA08") != std::string::npos) {
                SaveImage(savePath.c_str(), newTexture->mBuffer, newTexture->mWidth, newTexture->mHeight);
            }
        

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
