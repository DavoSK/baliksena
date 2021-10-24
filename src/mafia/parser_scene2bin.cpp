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

void DataFormatScene2BIN::readLm(MFUtil::ScopedBuffer& srcFile, Header* header, Object* object) {
    // int lmLevel = 1;
    // uint8_t unk1;
    // read(srcFile, &unk1);

    // if (!unk1)
    //     return;

    // uint32_t v6 = 0;
    // uint32_t v7 = 1;
    // uint32_t v8 = 8;

    // do
    // {
    //     if ( ((unsigned __int8)v7 & unk1) != 0 )
    //     ++v6;
    //     v7 *= 2;
    //     --v8;
    // }
    // while ( v8 );

    // unk1 = v6;

    // if ( !v6 )
    //     return;

    // struct LMStruct {
    //     uint8_t data[15];
    // };

    // while( 1 ) 
    // {
    //     LMStruct lm {};
    //     read(srcFile, &lm);


    // }
    // Logger::get().info("[LM] count: {}", v6);
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
            read(srcFile, object->mLightSectors, header->mSize);
        } break;

        case OBJECT_LIGHT_FLAGS: {
            read(srcFile, &object->mLightFlags);
        } break;

        case OBJECT_LIGHT_UNK_1: {
            read(srcFile, &object->mLightUnk0);
            read(srcFile, &object->mLightUnk1);
        } break;
    }
}

}  // namespace MFFormat
