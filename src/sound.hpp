#pragma once
#include "frame.hpp"
#include <string>
#include <vector>

struct SoundRadius {
    float InnerRadius;
    float OuterRadius;
    float InnerFalloff;
    float OuterFalloff;
};

enum class SoundType {
    Point, 
    Ambient,
    Unk1,
    Unk2
};

class Sound : public Frame {
public:
    ~Sound();
    
    [[nodiscard]] constexpr FrameType getFrameType() const override { return FrameType::Sound; }
    bool open(const std::string& path);

    void setLooping(bool loop) { mIsLooping = loop; mWasChanged = true; }
    [[nodiscard]] bool isLooping() const { return mIsLooping; }

    void setPitch(float pitch) { mPitch = pitch; mWasChanged = true; }
    [[nodiscard]] float getPitch() const { return mPitch; }

    void setVolume(float volume) { mVolume = volume; mWasChanged = true; }
    [[nodiscard]] float getVolume() const { return mVolume; }

    void setOutVolume(float volume) { mOutVolume = volume; }
    [[nodiscard]] float getOutVolume() const { return mOutVolume; }

    void setRadius(const SoundRadius& radius) { mRadius = radius; mWasChanged = true; }
    [[nodiscard]] const SoundRadius& getRadius() const {  return mRadius; }   

    void setType(SoundType type) { mSoundType = type; mWasChanged = true; }
    [[nodiscard]] const SoundType& getType() const { return mSoundType; }

    void setAbsPos(const glm::vec3& pos)  { mPos = pos; mWasChanged = true; }
    const glm::vec3& getAbsPos() const { return mPos; }

    void setCone(const glm::vec2& cone) { mCone = cone; mWasChanged = true; }
    const glm::vec2& getCone() const { return mCone; }

    void play();
    void pause();
    bool isPlaying() const { return mIsPlaying; }

    virtual void render() override;
private:
    glm::vec2 mCone;
    glm::vec3 mPos;
    SoundType mSoundType;
    std::string mFile;
    bool mWasChanged = false;
    bool mIsLooping = false;
    bool mIsPlaying = false;
    float mPitch = 1.0f;
    float mVolume = 0.0f;
    float mOutVolume = 0.0f;
    SoundRadius mRadius{};
    uint32_t mSourceHandle;     //ALuint
    uint32_t mBufferHandle;     //ALuint;
    friend class Audio;
};