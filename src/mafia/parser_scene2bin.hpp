#ifndef FORMAT_PARSERS_SCENE2_BIN_H
#define FORMAT_PARSERS_SCENE2_BIN_H

#include <cstring>
#include <optional>

#include "math.hpp"
#include "base_parser.hpp"
#include "utils.hpp"

namespace MFFormat
{

class DataFormatScene2BIN: public DataFormat
{
public:
    typedef enum {
        // top Headers
       HEADER_MISSION = 0x4c53,
       HEADER_META = 0x0001,
       HEADER_UNK_FILE = 0xAFFF,
       HEADER_UNK_FILE2 = 0x3200,
       HEADER_FOV = 0x3010,
       HEADER_VIEW_DISTANCE = 0x3011,
       HEADER_CLIPPING_PLANES = 0x3211,
       HEADER_WORLD = 0x4000,
       HEADER_SPECIAL_WORLD = 0xAE20,
       HEADER_ENTITIES = 0xAE20,
       HEADER_INIT = 0xAE50,
        // WORLD subHeader
       HEADER_OBJECT = 0x4010,
       HEADER_SPECIAL_OBJECT = 0xAE21,
    } HeaderType;

    typedef enum {
        OBJECT_TYPE_SPECIAL = 0xAE22,
        OBJECT_TYPE_NORMAL = 0x4011,
        OBJECT_TYPE_LM = 0x40A0,
        OBJECT_POSITION = 0x0020,
        OBJECT_ROTATION = 0x0022,
        OBJECT_POSITION_2 = 0x002C,
        OBJECT_SCALE = 0x002D,
        OBJECT_PARENT = 0x4020,
        OBJECT_HIDDEN = 0x4033,
        OBJECT_NAME = 0x0010, 
        OBJECT_NAME_SPECIAL = 0xAE23,
        OBJECT_MODEL = 0x2012,
        
        OBJECT_SOUND_MAIN = 0x4060,
        OBJECT_SOUND_TYPE = 0x4061,
        OBJECT_SOUND_VOLUME = 0x4062,
        OBJECT_SOUND_OUT_VOLUME = 0x4063, //f32
        OBJECT_SOUND_UNK2 = 0x4064, //vec2
        OBJECT_SOUND_RADIUS = 0x4068,
        OBJECT_SOUND_LOOP = 0x4066,
        OBJECT_SOUND_PITCH = 0xb800,
        OBJECT_SOUND_SECTOR = 0xb200,

        OBJECT_LIGHT_MAIN = 0x4040,
        OBJECT_LIGHT_TYPE = 0x4041,
        OBJECT_LIGHT_COLOUR = 0x0026,
        OBJECT_LIGHT_POWER = 0x4042,
        OBJECT_LIGHT_CONE = 0x4043,
        OBJECT_LIGHT_RANGE = 0x4044,
        OBJECT_LIGHT_FLAGS = 0x4045,
        OBJECT_LIGHT_SECTOR= 0x4046,
        OBJECT_SPECIAL_DATA= 0xAE24,
    } ObjectProperty;

    typedef enum {
        OBJECT_TYPE_LIGHT = 0x02,
        OBJECT_TYPE_CAMERA = 0x03,
        OBJECT_TYPE_SOUND = 0x04,
        OBJECT_TYPE_MODEL = 0x09,
        OBJECT_TYPE_OCCLUDER = 0x0C,
        OBJECT_TYPE_SECTOR = 0x99,
        OBJECT_TYPE_SCRIPT = 0x9B,
        OBJECT_TYPE_LAST
    } ObjectType;

    typedef enum {
        SPECIAL_OBJECT_TYPE_NONE = 0,
        SPECIAL_OBJECT_TYPE_PHYSICAL = 0x23,
        SPECIAL_OBJECT_TYPE_PLAYER = 0x02,
        SPECIAL_OBJECT_TYPE_CHARACTER = 0x1B,
        SPECIAL_OBJECT_TYPE_CAR = 0x06,
        SPECIAL_OBJECT_TYPE_PUB_VEHICLE = 0x08,
        SPECIAL_OBJECT_TYPE_SCRIPT = 0x05,
    } SpecialObjectType;

    typedef enum {
        LIGHT_TYPE_POINT = 0x01,
        LIGHT_TYPE_SPOT = 0x02,
        LIGHT_TYPE_DIRECTIONAL = 0x03,
        LIGHT_TYPE_AMBIENT = 0x04,
        LIGHT_TYPE_FOG = 0x05,
        LIGHT_TYPE_POINT_AMBIENT = 0x06,
        LIGHT_TYPE_LAYERED_FOG = 0x08,
    } LightType;

    #pragma pack(push, 1)
    typedef struct
    {
        uint16_t mType;
        uint32_t mSize;
    } Header;
    #pragma pack(pop)

    //NOTE: lmap structs
    typedef enum BitmapType : uint8_t
    {
        LM_BITMAP_TYPE_BITMAP = 0,
        LM_BITMAP_TYPE_COLOR = 1,
    } BitmapType;

    typedef enum : uint8_t
    {
        LM_VERTEX = 1 << 0,
        LM_BITMAP = 1 << 1,
        LM_BUILD = 1 << 2,
    } LightmapLevelFlags;

    struct VertexLightmap
    {
        uint32_t mNumVertices;
        std::vector<uint32_t> mData;
    };

    struct BitmapPixel
    {
        uint8_t mData[3];
    };

    struct Bitmap
    {
        uint32_t mWidth;
        uint32_t mHeight;
        //NOTE: optional field depends on type
        //if single color no data is used
        std::optional<std::vector<BitmapPixel>> mData;
    };

    struct BitmapGroup
    {
        BitmapType mType;
        //NOTE: optional field depends on type
        std::optional<uint32_t> mColor;
        uint32_t mNumBitmaps;
        std::vector<Bitmap> mBitmaps;
    };

    struct BitmapLightmapVertex
    {
        float mU;
        float mV;
    };

    struct FaceGroup
    {
        uint32_t mNumFaces;
        std::vector<uint16_t> mBitmapIndices;
    };

    struct BitmapLightmap
    {
        uint16_t mNumBitmapGroups;
        uint16_t mNumFaceGroups;
        std::vector<BitmapGroup> mBitmapGroups; //NumBitmapGroups
        uint32_t mNumVertices;
        std::vector<BitmapLightmapVertex> mVertices; //NumVertices
        std::vector<uint32_t> mVertexBitmapGroupTable; //NumVertices
        uint32_t mNumIndices;
        std::vector<uint16_t> mIndices; //NumIndices
        std::vector<FaceGroup> mFaceGroups; //NumFaceGroups
    };
    
    struct LodLevel
    {
        uint16_t mNumVertices;
        std::optional<VertexLightmap> mVertexLightmapData;
        std::optional<BitmapLightmap> mBitmapLightmapData;
    };

    struct LightmapLevel
    {
        //NOTE: always 0x21
        uint8_t mVersion;
        LightmapLevelFlags mFlags;
        //NOTE: should match the associated mesh's lod count
        uint32_t mNumLods;
        float mResolution;
        float mUnk;
        uint8_t mLevel;
        std::vector<LodLevel> mLodLevels; //NumLods
    };

    struct Lightmap
    {
        //NOTE: bitmap indicating the lightmap levels covered by this lightmap
        uint8_t mLightmapLevels;
        std::vector<LightmapLevel> mLevels; //num_set_bits(mLightmapLevels)
    };

    typedef struct _Object
    {
        bool mIsPatch = true;
        bool mIsHidden = false;
        uint32_t mType;
        uint32_t mSpecialType;
        MFMath::Vec3 mPos;
        bool mIsPosPatched = false;
        MFMath::Quat mRot;
        bool mIsRotPatched = false;
        MFMath::Vec3 mPos2; // precomputed final world transform position
        bool mIsPos2Patched = false;
        MFMath::Vec3 mScale;
        bool mIsScalePatched = false;
        std::string mName;
        std::string mModelName;
        std::string mParentName;
        bool mIsParentPatched = false;

        // Light properties
        LightType mLightType;
        MFMath::Vec3 mLightColour;
        int32_t mLightFlags;
        float mLightPower;           // 1.0 = 100% (can be even over 1.0)
        float mLightConeTheta;
        float mLightConePhi;
        float mLightNear;
        float mLightFar;
        std::vector<std::string> mLightSectors;

        // Sound properties
        struct {
            std::string mFile;
            uint32_t mType;
            float mVolume;
            float mOutVolume;
            MFMath::Vec2 mCone;
            struct {
                float InnerRadius;
                float OuterRadius;
                float InnerFalloff;
                float OuterFalloff;
            } mRadius;
            bool mLoop;
            float mPitch;
            std::vector<std::string> mSectors;
        } mSound;

        struct {
            // Physical object properties
            float mMovVal1;
            float mMovVal2;
            float mFriction;
            float mMovVal4;

            int32_t mMovVal5;
            float mWeight;
            int32_t mSound;
        } mSpecialProps;

        std::optional<Lightmap> mLightMap;
    } Object;

    virtual bool load(MFUtil::ScopedBuffer&srcFile);
    inline size_t getNumObjects()                               { return mObjects.size(); }
    inline Object* getObject(std::string name)                  { return &mObjects.at(name); }
    inline std::unordered_map<std::string, Object> getObjects() { return mObjects; }
    inline std::unordered_map<std::string, Object> getExternalObjects() { return mExternalObjects; }
    inline float getFov()                                       { return mFov; }
    inline void setFov(float value)                             { mFov = value; }
    inline float getViewDistance()                              { return mViewDistance; }
    inline void setViewDistance(float value)                    { mViewDistance = value; }
    inline MFMath::Vec2  getClippingPlanes()                    { return mClippingPlanes; }
    inline void  setClippingPlanes(MFMath::Vec2 value)          { mClippingPlanes = value; }
    static std::string lightTypeToStr(LightType t);

private:
    void readHeader(MFUtil::ScopedBuffer&srcFile, Header* header, uint32_t offset);
    void readObject(MFUtil::ScopedBuffer&srcFile, Header* header, Object* object, uint32_t offset);
    void readSound(MFUtil::ScopedBuffer&srcFile, Header* header, Object* object);
    void readLight(MFUtil::ScopedBuffer&srcFile, Header* header, Object* object);

    VertexLightmap readLmVertexData(MFUtil::ScopedBuffer& srcFile);

    Bitmap readLmBitmap(MFUtil::ScopedBuffer& srcFile, BitmapType type);
    BitmapGroup readLmBitmapGroup(MFUtil::ScopedBuffer& srcFile);
    BitmapLightmap readLmBitmapData(MFUtil::ScopedBuffer& srcFile);
    LodLevel readLmLodLevel(MFUtil::ScopedBuffer& srcFile, uint8_t flags);
    LightmapLevel readLmLevel(MFUtil::ScopedBuffer& srcFile);
    void readLm(MFUtil::ScopedBuffer& srcFile, Header* header, Object* object);
    
    std::unordered_map<std::string, Object> mObjects;
    std::unordered_map<std::string, Object> mExternalObjects;
    float mFov;
    float mViewDistance;
    MFMath::Vec2  mClippingPlanes;
};

}

#endif
