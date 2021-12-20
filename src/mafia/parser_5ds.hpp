#ifndef FORMAT_PARSERS_5DS_H
#define FORMAT_PARSERS_5DS_H

#include "base_parser.hpp"
#include "math.hpp"

namespace MFFormat
{

class DataFormat5DS: public DataFormat
{
public:
    virtual bool load(MFUtil::ScopedBuffer& srcFile) override;

    typedef enum
    {
        SEQUENCE_MOVEMENT = 0x2,
        SEQUENCE_ROTATION = 0x4,
        SEQUENCE_SCALE = 0x8
    } TypeOfSequence;

    #pragma pack(push,1)
    typedef struct
    {
        // should be "5DS\0" 
        uint32_t mMagicByte; 
        // should be 0x14
        uint16_t mAnimationType;
        uint32_t mUnk1;
        uint32_t mUnk2;
        uint32_t mLengthOfAnimationData;
    } Header;
   
    typedef struct
    {
        uint16_t mNumberOfAnimatedObjects;
        // Note: 25 frames = 1 seconds
        uint16_t mOverallCountOfFrames;
    } Description;
    
    typedef struct
    {
        uint32_t mPointerToString;
        uint32_t mPointerToData;
    } PointerTable;

    #pragma pack(pop)

    class AnimationSequence
    {
    public:
        void setNumberOfSequences(uint16_t numberOfSequences);
        void setType(uint16_t type);
        void setName(const std::string& str);
        void addTimestamp(uint32_t time);
        void addMovement(MFMath::Vec3& data);
        void addMovementKey(uint16_t key) { mMovementsKeys.push_back(key); }
        void addRotation(MFMath::Quat& data);
        void addRotationKey(uint16_t key) { mRotationsKeys.push_back(key); }
        void addScale(MFMath::Vec3& data);
        void addScaleKey(uint16_t key) { mScaleKeys.push_back(key); }
        const std::string getName() const;
        uint16_t getCount() const;
        const uint32_t& getTimestamp(uint16_t id) const;
        const std::vector<MFMath::Vec3>& getTranslations() const { return mMovements; }
        const std::vector<MFMath::Quat>& getRotations() const { return mRotations; }
        const std::vector<MFMath::Vec3>& getScales() const { return mScale; }
        bool hasMovement() const;
        bool hasRotation() const;
        bool hasScale() const;
    private:
        std::string mObjectName;
        std::vector<uint32_t> mTimestamps;
        std::vector<MFMath::Vec3> mMovements;
        std::vector<uint16_t> mMovementsKeys;
        std::vector<MFMath::Quat> mRotations;
        std::vector<uint16_t> mRotationsKeys;
        std::vector<MFMath::Vec3> mScale;
        std::vector<uint16_t> mScaleKeys;

        uint16_t mNumberOfSequences;
        TypeOfSequence mType;
    };

    const std::vector<AnimationSequence>& getSequences() const { return mSequences; }
    unsigned int getTotalFrameCount() const { return mTotalFrameCount; }
private:
    bool parseAnimationSequence(MFUtil::ScopedBuffer& inputFile, uint32_t pointerData, uint32_t pointerName);
    std::vector<AnimationSequence> mSequences;
    unsigned int mTotalFrameCount;
};

}

#endif
