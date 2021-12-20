#include "parser_5ds.hpp"

namespace MFFormat
{

void DataFormat5DS::AnimationSequence::setNumberOfSequences(uint16_t numberOfSequences)
{
    mNumberOfSequences = numberOfSequences;    
}

void DataFormat5DS::AnimationSequence::setType(uint16_t type)
{
    mType = static_cast<TypeOfSequence>(type); 
}

void DataFormat5DS::AnimationSequence::setName(const std::string& str)
{
    mObjectName = str;
}

void DataFormat5DS::AnimationSequence::addTimestamp(uint32_t time)
{
    mTimestamps.push_back(time);
}

void DataFormat5DS::AnimationSequence::addMovement(MFMath::Vec3& data)
{
    mMovements.push_back(data);
}

void DataFormat5DS::AnimationSequence::addRotation(MFMath::Quat& data)
{
    mRotations.push_back(data);
}

void DataFormat5DS::AnimationSequence::addScale(MFMath::Vec3& data)
{
    mScale.push_back(data);
}

const std::string DataFormat5DS::AnimationSequence::getName() const
{
    return mObjectName;
}

uint16_t DataFormat5DS::AnimationSequence::getCount() const
{
    return mNumberOfSequences;
}

const uint32_t& DataFormat5DS::AnimationSequence::getTimestamp(uint16_t id) const
{ 
    return mTimestamps[id];
}

bool DataFormat5DS::AnimationSequence::hasMovement() const
{ 
    return (mType & SEQUENCE_MOVEMENT);
}

bool DataFormat5DS::AnimationSequence::hasRotation() const
{ 
    return (mType & SEQUENCE_ROTATION);
}

bool DataFormat5DS::AnimationSequence::hasScale() const
{ 
    return (mType & SEQUENCE_SCALE);
}

bool DataFormat5DS::parseAnimationSequence(MFUtil::ScopedBuffer& inputFile, uint32_t pointerData, uint32_t pointerName)
{
    AnimationSequence result;

    //seek to destination
    inputFile.seek(pointerData);

    // read block type; 
    uint32_t typeOfBlock;
    read(inputFile, &typeOfBlock);
    result.setType(typeOfBlock);

    if(typeOfBlock & SEQUENCE_ROTATION) {
        uint16_t numRotationKeys;
        read(inputFile, &numRotationKeys);

        for(size_t i = 0; i < numRotationKeys; i++) {
            uint16_t rotKey;
            read(inputFile, &rotKey);
            result.addRotationKey(rotKey);
        }

        if (numRotationKeys % 2 == 0) {
            inputFile.seek(2, 0);
        }

        for (size_t i = 0; i < numRotationKeys; i++) {
            MFMath::Quat rotationData;
            read(inputFile, &rotationData);
            rotationData.fromMafia();
            result.addRotation(rotationData);
        }
    }

    if(typeOfBlock & SEQUENCE_MOVEMENT) {
        uint16_t numTransKeys;
        read(inputFile, &numTransKeys);

        for(size_t i = 0; i < numTransKeys; i++) {
            uint16_t transKey;
            read(inputFile, &transKey);
            result.addMovementKey(transKey);
        }

        if (numTransKeys % 2 == 0) {
            inputFile.seek(2, 0);
        }

        for (size_t i = 0; i < numTransKeys; i++) {
            MFMath::Vec3 movementData;
            read(inputFile, &movementData);
            result.addMovement(movementData);
        }
    }

    if(typeOfBlock & SEQUENCE_SCALE) {
        uint16_t numScaleKeys;
        read(inputFile, &numScaleKeys);

        for(size_t i = 0; i < numScaleKeys; i++) {
            uint16_t scaleKey;
            read(inputFile, &scaleKey);
            result.addScaleKey(scaleKey);
        }

        if (numScaleKeys % 2 == 0) {
            inputFile.seek(2, 0);
        }

        for (size_t i = 0; i < numScaleKeys; i++) {
            MFMath::Vec3 scaleData;
            read(inputFile, &scaleData);
            result.addScale(scaleData);
        }
    }

    // get string
    inputFile.seek(pointerName); 

    std::string objectName;
    char objectNameChar;
    inputFile.read(&objectNameChar, 1);

    while(objectNameChar) {
        objectName.push_back(objectNameChar);
        inputFile.read(&objectNameChar, 1);
    }
    
    result.setName(objectName); 
    mSequences.push_back(result);
    return true;
}

bool DataFormat5DS::load(MFUtil::ScopedBuffer &srcFile) 
{
    Header new_header = {};
    read(srcFile, &new_header);

    if(new_header.mMagicByte != 0x00534435)
    {
        //NOTE(DavoSK): add event handler 
        return false;
    }

    auto begginingOfData = srcFile.tellg();
    Description new_desc = {};
    read(srcFile, &new_desc);
    mTotalFrameCount = new_desc.mOverallCountOfFrames;
    PointerTable new_pointer_table = {};

    for(unsigned int i = 0; i < new_desc.mNumberOfAnimatedObjects; i++)
    {
        read(srcFile, &new_pointer_table);
        auto nextBlock = srcFile.tellg();

        uint32_t pointerToName = static_cast<uint32_t>(((uint32_t) begginingOfData) + new_pointer_table.mPointerToString);
        uint32_t pointerToData = static_cast<uint32_t>(((uint32_t) begginingOfData) + new_pointer_table.mPointerToData);
        parseAnimationSequence(srcFile, pointerToData, pointerToName);
        srcFile.seek(nextBlock);
    }

    return true;
}

}
