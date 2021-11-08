#include "parser_scene2bin.hpp"
#include "../logger.hpp"

namespace MFFormat {

std::string DataFormatScene2BIN::lightTypeToStr(LightType t) {
    switch (t) {
        case MFFormat::DataFormatScene2BIN::LIGHT_TYPE_POINT:
            return "point";
            break;
        case MFFormat::DataFormatScene2BIN::LIGHT_TYPE_DIRECTIONAL:
            return "directional";
            break;
        case MFFormat::DataFormatScene2BIN::LIGHT_TYPE_AMBIENT:
            return "ambient";
            break;
        case MFFormat::DataFormatScene2BIN::LIGHT_TYPE_FOG:
            return "fog";
            break;
        case MFFormat::DataFormatScene2BIN::LIGHT_TYPE_POINT_AMBIENT:
            return "point ambient";
            break;
        case MFFormat::DataFormatScene2BIN::LIGHT_TYPE_LAYERED_FOG:
            return "layered fog";
            break;
        default:
            break;
    }

    return "unknown";
}

bool DataFormatScene2BIN::load(MFUtil::ScopedBuffer& srcFile) {
    Header newHeader = {};
    read(srcFile, &newHeader);
    uint32_t position = 6;

    while (position + 6 < newHeader.mSize) {
        srcFile.seek(position);
        Header nextHeader = {};
        read(srcFile, &nextHeader);
        readHeader(srcFile, &nextHeader, position + 6);
        position += nextHeader.mSize;
    }

    return true;
}

void DataFormatScene2BIN::readHeader(MFUtil::ScopedBuffer& srcFile, Header* header, uint32_t offset) {
    switch (header->mType) {
        case HEADER_SPECIAL_WORLD:
        case HEADER_WORLD: {
            uint32_t position = offset;
            while (position + 6 < offset + header->mSize) {
                Header nextHeader = {};
                srcFile.seek(position);
                read(srcFile, &nextHeader);
                readHeader(srcFile, &nextHeader, position + 6);
                position += nextHeader.mSize;
            }
        } break;

        case HEADER_VIEW_DISTANCE: {
            read(srcFile, &mViewDistance);
        } break;

        case HEADER_CLIPPING_PLANES: {
            read(srcFile, &mClippingPlanes);
        } break;

        case HEADER_FOV: {
            read(srcFile, &mFov);
        } break;

        case HEADER_SPECIAL_OBJECT:
        case HEADER_OBJECT: {
            uint32_t position = offset;
            Object newObject = {};
            while (position + 6 < offset + header->mSize) {
                Header nextHeader = {};
                srcFile.seek(position);
                read(srcFile, &nextHeader);
                readObject(srcFile, &nextHeader, &newObject, position + 6);
                position += nextHeader.mSize;
            }

            if (header->mType == HEADER_OBJECT) {
                mObjects.insert(make_pair(newObject.mName, newObject));
            } else {
                auto object_it = mObjects.find(newObject.mName);

                if (object_it != mObjects.end()) {
                    auto object = &object_it->second;
                    object->mSpecialType = newObject.mSpecialType;

                    memcpy(&object->mSpecialProps, &newObject.mSpecialProps, sizeof(newObject.mSpecialProps));
                } else {
                    mExternalObjects.insert(make_pair(newObject.mName, newObject));
                }
            }
        } break;
    }
}

DataFormatScene2BIN::VertexLightmap DataFormatScene2BIN::readLmVertexData(MFUtil::ScopedBuffer& srcFile) {
    VertexLightmap vertexLm{};
    read(srcFile, &vertexLm.mNumVertices);
    for(size_t i = 0; i < vertexLm.mNumVertices; i++) {
        uint32_t vertex;
        read(srcFile, &vertex);
        vertexLm.mData.push_back(vertex);
    }

    return vertexLm;
}

DataFormatScene2BIN::Bitmap DataFormatScene2BIN::readLmBitmap(MFUtil::ScopedBuffer& srcFile, BitmapType type) {
    Bitmap bitmap{};
    read(srcFile, &bitmap.mWidth);
    read(srcFile, &bitmap.mHeight);

    if(type == LM_BITMAP_TYPE_BITMAP) {
        std::vector<BitmapPixel> pixels;
        for(size_t i = 0; i < bitmap.mWidth * bitmap.mHeight; i++) {
            BitmapPixel pixel;
            read(srcFile, &pixel);
            pixels.push_back(pixel);
           
        }

        bitmap.mData = pixels;
    }
    return bitmap;
}

DataFormatScene2BIN::BitmapGroup DataFormatScene2BIN::readLmBitmapGroup(MFUtil::ScopedBuffer& srcFile) {
    BitmapGroup bitmapGroup{};
    read(srcFile, &bitmapGroup.mType);

    if(bitmapGroup.mType == LM_BITMAP_TYPE_COLOR) {
        uint32_t color = 0x0;
        read(srcFile, &color);
        bitmapGroup.mColor = color;
    }

    read(srcFile, &bitmapGroup.mNumBitmaps);

    for(size_t i = 0; i < bitmapGroup.mNumBitmaps; i++) {
        bitmapGroup.mBitmaps.push_back(readLmBitmap(srcFile, bitmapGroup.mType));
    }

    return bitmapGroup;
}

DataFormatScene2BIN::BitmapLightmap DataFormatScene2BIN::readLmBitmapData(MFUtil::ScopedBuffer& srcFile) {
    BitmapLightmap bitmapLm{};
    read(srcFile, &bitmapLm.mNumBitmapGroups);
    read(srcFile, &bitmapLm.mNumFaceGroups);

    for(size_t i = 0; i < bitmapLm.mNumBitmapGroups; i++) {
        bitmapLm.mBitmapGroups.push_back(readLmBitmapGroup(srcFile));
    }

    read(srcFile, &bitmapLm.mNumVertices);
    for(size_t i = 0; i < bitmapLm.mNumVertices; i++) {
        BitmapLightmapVertex vertex {};
        read(srcFile, &vertex);
        bitmapLm.mVertices.push_back(vertex);
    }

    for(size_t i = 0; i < bitmapLm.mNumVertices; i++) {
        uint32_t groupTable;
        read(srcFile, &groupTable);
        bitmapLm.mVertexBitmapGroupTable.push_back(groupTable);
    }

    read(srcFile, &bitmapLm.mNumIndices);

    for(size_t i = 0; i < bitmapLm.mNumIndices; i++) {
        uint16_t indice;
        read(srcFile, &indice);
        bitmapLm.mIndices.push_back(indice);
    }

    for(size_t i = 0; i < bitmapLm.mNumFaceGroups; i++) {
        FaceGroup faceGroup{};
        read(srcFile, &faceGroup.mNumFaces);
        for(size_t j = 0; j < faceGroup.mNumFaces; j++) {
            uint16_t fgroupIndice;
            read(srcFile, &fgroupIndice);
            faceGroup.mBitmapIndices.push_back(fgroupIndice);
        }
        bitmapLm.mFaceGroups.push_back(faceGroup);
    }

    return bitmapLm;
}

DataFormatScene2BIN::LodLevel DataFormatScene2BIN::readLmLodLevel(MFUtil::ScopedBuffer& srcFile, uint8_t flags) {
    LodLevel lodLevel{};
    read(srcFile, &lodLevel.mNumVertices);

    if (flags & LM_VERTEX) {
        lodLevel.mVertexLightmapData = readLmVertexData(srcFile);
    }
    else if (flags & LM_BITMAP){
         lodLevel.mBitmapLightmapData = readLmBitmapData(srcFile);
    }

    return lodLevel;
}

DataFormatScene2BIN::LightmapLevel DataFormatScene2BIN::readLmLevel(MFUtil::ScopedBuffer& srcFile) {
    LightmapLevel level{};  
    read(srcFile, &level.mVersion);
    assert(level.mVersion == 33); 
    read(srcFile, &level.mFlags);
    read(srcFile, &level.mNumLods);
    read(srcFile, &level.mResolution);
    read(srcFile, &level.mUnk);
    read(srcFile, &level.mLevel);
    
    for(size_t i = 0; i < level.mNumLods; i++) {
        level.mLodLevels.push_back(readLmLodLevel(srcFile, (uint8_t)level.mFlags));
    }

    return level;
}

int countSetBits(uint8_t n){
    if (n == 0)
        return 0;
    else
        return (n & 1) + countSetBits(n >> 1);
}

void DataFormatScene2BIN::readLm(MFUtil::ScopedBuffer& srcFile, Header* header, Object* object) {
    Lightmap lightMap = {};
    read(srcFile, &lightMap.mLightmapLevels);

    const auto lmLevels = countSetBits(lightMap.mLightmapLevels);
    for(size_t i = 0; i < lmLevels; i++) {
        lightMap.mLevels.push_back(readLmLevel(srcFile));
    }

    object->mLightMap = lightMap;    
}  

void DataFormatScene2BIN::readObject(MFUtil::ScopedBuffer& srcFile, Header* header, Object* object, uint32_t offset) {
    switch (header->mType) {
        case OBJECT_TYPE_SPECIAL: {
            read(srcFile, &object->mSpecialType);
        } break;

        case OBJECT_TYPE_LM: {
           readLm(srcFile, header, object);
        } break;

        case OBJECT_TYPE_NORMAL: {
            read(srcFile, &object->mType);

            if(object->mType > 0 && object->mType < OBJECT_TYPE_LAST)
                object->mIsPatch = false;
        } break;

        case OBJECT_NAME:
        case OBJECT_NAME_SPECIAL: {
            char* name = reinterpret_cast<char*>(malloc(header->mSize + 1));
            read(srcFile, name, header->mSize - 1);
            name[header->mSize] = '\0';

            object->mName = std::string(name);
            free(name);
        } break;

        case OBJECT_SPECIAL_DATA: {
            switch (object->mSpecialType) {
                case SPECIAL_OBJECT_TYPE_PHYSICAL: {
                    srcFile.seek(2, srcFile.cur);

                    read(srcFile, &object->mSpecialProps.mMovVal1);
                    read(srcFile, &object->mSpecialProps.mMovVal2);
                    read(srcFile, &object->mSpecialProps.mWeight);
                    read(srcFile, &object->mSpecialProps.mFriction);
                    read(srcFile, &object->mSpecialProps.mMovVal4);
                    read(srcFile, &object->mSpecialProps.mSound);

                    srcFile.seek(1, srcFile.cur);

                    read(srcFile, &object->mSpecialProps.mMovVal5);
                } break;
            }
        } break;

        case OBJECT_MODEL: {
            char* modelName = reinterpret_cast<char*>(malloc(header->mSize + 1));
            read(srcFile, modelName, header->mSize);
            modelName[strlen(modelName) - 4] = '\0';
            sprintf(modelName, "%s.4ds", modelName);
            modelName[header->mSize] = '\0';

            object->mModelName = std::string(modelName);
            free(modelName);
        } break;

        case OBJECT_HIDDEN: {
            object->mIsHidden = true;
        } break;

        case OBJECT_POSITION: {
            MFMath::Vec3 newPosition = {};
            read(srcFile, &newPosition);
            object->mPos = newPosition;
            object->mIsPosPatched = true;
        } break;

        case OBJECT_ROTATION: {
            MFMath::Quat newRotation = {};
            read(srcFile, &newRotation);
            newRotation.fromMafia();
            object->mRot = newRotation;
            object->mIsRotPatched = true;
        } break;

        case OBJECT_POSITION_2: {
            MFMath::Vec3 newPosition = {};
            read(srcFile, &newPosition);
            object->mPos2 = newPosition;
            object->mIsPos2Patched = true;
        } break;

        case OBJECT_SCALE: {
            MFMath::Vec3 newScale = {};
            read(srcFile, &newScale);
            object->mScale = newScale;
            object->mIsScalePatched = true;
        } break;

        case OBJECT_LIGHT_MAIN: {
            uint32_t position = offset;
            while (position + 6 < offset + header->mSize) {
                Header lightHeader = {};
                read(srcFile, &lightHeader);
                readLight(srcFile, &lightHeader, object);
                position += lightHeader.mSize;
            }
        } break;

        case OBJECT_PARENT: {
            Header parentHeader = {};
            read(srcFile, &parentHeader);
            Object parentObject = {};
            readObject(srcFile, &parentHeader, &parentObject, offset + 6);

            object->mParentName = parentObject.mName;
            object->mIsParentPatched = true;
        } break;
    }
}

void DataFormatScene2BIN::readLight(MFUtil::ScopedBuffer& srcFile, Header* header, Object* object) {
    switch (header->mType) {
        case OBJECT_LIGHT_TYPE: {
            read(srcFile, &object->mLightType);
        } break;

        case OBJECT_LIGHT_COLOUR: {
            read(srcFile, &object->mLightColour);
        } break;

        case OBJECT_LIGHT_POWER: {
            read(srcFile, &object->mLightPower);
        } break;

        case OBJECT_LIGHT_RANGE: {
            read(srcFile, &object->mLightNear);
            read(srcFile, &object->mLightFar);
        } break;

        case OBJECT_LIGHT_SECTOR: {
            char* sectorName = (char*)malloc(header->mSize - 6);
            read(srcFile, sectorName, header->mSize - 6);
            object->mLightSectors.push_back(sectorName);
            free(sectorName);
        } break;

        case OBJECT_LIGHT_FLAGS: {
            read(srcFile, &object->mLightFlags);
        } break;

        case OBJECT_LIGHT_CONE: {
            read(srcFile, &object->mLightConeTheta);
            read(srcFile, &object->mLightConePhi);
        } break;
    }
}

}  // namespace MFFormat
