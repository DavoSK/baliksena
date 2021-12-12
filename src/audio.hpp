#pragma once
struct ALCdevice;
struct ALCcontext;

class Sector;
class Sound;
class Audio {
public:
    void init();
    void update();
    void open(Sound* sound);
    void update(Sound* sound);
private:
    Sector *mPrevSector { nullptr };
    
    ALCdevice *mDevice { nullptr };
    ALCcontext *mContext { nullptr };
    bool mInited { false };
};