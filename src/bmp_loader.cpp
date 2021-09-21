#include "bmp_loader.hpp"

#define BI_RGB 0L
#pragma pack(push, 1)
typedef struct tagBITMAPFILEHEADER {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
    uint8_t rgbBlue;
    uint8_t rgbGreen;
    uint8_t rgbRed;
    uint8_t rgbReserved;
} RGBQUAD;
#pragma pack(pop)

inline bool areColorSimilar(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2) { return r1 == r2 && g1 == g2 && b1 == b2; }

uint8_t* loadBMP(std::ifstream& file, int* w, int* h, bool useTransparencyKey) {
    BITMAPFILEHEADER bmpFileHeader;
    file.read((char*)&bmpFileHeader, sizeof(BITMAPFILEHEADER));

    // NOTE: not valid BMP !
    if (bmpFileHeader.bfType != 0x4D42) return nullptr;

    BITMAPINFOHEADER bmmpInfoHeader;
    file.read((char*)&bmmpInfoHeader, sizeof(BITMAPINFOHEADER));

    bool is8Byte = false;
    RGBQUAD bmiColors[256];

    // NOTE(DavoSK): all mafia textures are not compressed !
    if (bmmpInfoHeader.biCompression != BI_RGB) 
        return nullptr;

    // NOTE(DavoSK): mafia has only 2 types of textures
    // 8 byte texture -> simple array of indices into color pallet that is
    // located after BITMAPINFOHEADER and 24 byte texture that is even simplier (
    // linear array of BRG components )
    if (bmmpInfoHeader.biBitCount == 8) {
        const auto palleteSize = bmmpInfoHeader.biClrUsed ? bmmpInfoHeader.biClrUsed : 1 << bmmpInfoHeader.biBitCount;
        file.read((char*)bmiColors, sizeof(RGBQUAD) * palleteSize);
        is8Byte = true;
    } else {
        file.read((char*)bmiColors, sizeof(RGBQUAD));
    }

    file.seekg(bmpFileHeader.bfOffBits);

    // NOTE(DavoSK): mafia uses first key color in pallete in 8 bit texture as
    // alpha dunno about 24 bit ones
    RGBQUAD keyColor = bmiColors[0];
    const auto originalImageSize = bmmpInfoHeader.biWidth * bmmpInfoHeader.biHeight * 3;
    const auto rgbaImageSize = bmmpInfoHeader.biWidth * bmmpInfoHeader.biHeight * 4;

    // NOTE(DavoSK): since we are in modern days we ussualy want RGBA output
    // format we are doin simple comparison of color to keyColor to set A channel
    // also swapping first channels to RGB order
    uint8_t* buffer = new uint8_t[rgbaImageSize];
    if (is8Byte) {
        const auto indexedSize = bmmpInfoHeader.biWidth * bmmpInfoHeader.biHeight;
        uint8_t* indices = new uint8_t[indexedSize];
        file.read((char*)indices, indexedSize);

        for (size_t i = 0, j = 0; i < indexedSize; i++, j += 4) {
            auto pixel = bmiColors[indices[i]];
            buffer[j] = pixel.rgbRed;
            buffer[j + 1] = pixel.rgbGreen;
            buffer[j + 2] = pixel.rgbBlue;
            buffer[j + 3] = useTransparencyKey &&
                                    areColorSimilar(pixel.rgbBlue, pixel.rgbGreen, pixel.rgbRed, keyColor.rgbBlue, keyColor.rgbGreen, keyColor.rgbRed)
                                ? 0
                                : 255;
        }
    } else {
        uint8_t* rgbBuffer = new uint8_t[originalImageSize];
        file.read((char*)rgbBuffer, originalImageSize);
        for (size_t i = 0, j = 0; i < originalImageSize; i += 3, j += 4) {
            buffer[j] = rgbBuffer[i + 2];
            buffer[j + 1] = rgbBuffer[i + 1];
            buffer[j + 2] = rgbBuffer[i];
            buffer[j + 3] = useTransparencyKey && areColorSimilar(rgbBuffer[i + 2], rgbBuffer[i + 1], rgbBuffer[i], keyColor.rgbRed,
                                                                  keyColor.rgbGreen, keyColor.rgbBlue)
                                ? 0
                                : 255;
        }
    }

    *w = bmmpInfoHeader.biWidth;
    *h = bmmpInfoHeader.biHeight;
    return buffer;
}

uint8_t* loadBMPEx(const char* fileName, int* w, int* h, bool useTransparencyKey)  {
    std::ifstream inputFile(fileName, std::ifstream::binary);
    if(inputFile.good()) {
        return loadBMP(inputFile, w, h, useTransparencyKey);
    }
    return nullptr;
}
