#include "audio.hpp"
#include "logger.hpp"

#include "app.hpp"
#include "scene.hpp"
#include "camera.h"
#include "sound.hpp"
#include "vfs.hpp"
#include "sector.hpp"

#include <AL/al.h>;
#include <AL/alc.h>

void Audio::init() {
    mDevice = alcOpenDevice(nullptr);
    if (!mDevice) {
        Logger::get().info("alcOpenDevice {}", alGetError());
        Logger::get().error("unable to get default audio device :/ !");
        return;
    }
    
    mContext = alcCreateContext(mDevice, nullptr);
    if (!alcMakeContextCurrent(mContext)) {
        // failed to make context current
        // test for errors here using alGetError();
        Logger::get().error("unable to create context for default audio device :/ !");
        return;
    }

    Logger::get().info("OpenAl initialized !");
    mInited = true;
}

void Audio::doSectorTransition() {
    constexpr float loweringStep = 0.01f;
    
    bool soundsMuted = false;
    if(mPrevSector != nullptr) {
        size_t mutedCount = 0;
        const auto& prevSectorSounds = mPrevSector->getSounds();
        for(auto& sound : prevSectorSounds) {
            float currentSoundVolume = sound->getVolume();
            currentSoundVolume -= loweringStep;
            if(currentSoundVolume < 0.0f) {
                currentSoundVolume = 0.0f;

                if(sound->isPlaying()) {
                    sound->pause();
                }

                mutedCount++;
            }

            //Logger::get().info("Sector sound set volume: {}", currentSoundVolume);
            sound->setVolume(currentSoundVolume);
            //Logger::get().info("Sector sound get volume: {}", sound->getVolume());
        }

        soundsMuted = mutedCount == prevSectorSounds.size();
        //Logger::get().info("Muting {} / {}", mutedCount, prevSectorSounds.size());
    } else {
        soundsMuted = true;
    }

    bool soundsResumed = false;
    if(mCurrentSector != nullptr) {
        size_t resumedCount = 0;
        const auto& currSectorSounds = mCurrentSector->getSounds();
        for(auto& sound : currSectorSounds) {
            float currentSoundVolume = sound->getVolume();
            currentSoundVolume += loweringStep;

            if(!sound->isPlaying()) {
                sound->play();
            }

            if(currentSoundVolume >= sound->getOutVolume()) {
                currentSoundVolume = sound->getOutVolume();
                resumedCount++;
            }

            sound->setVolume(currentSoundVolume);
        }

        soundsResumed = resumedCount == currSectorSounds.size();
    } else {
        soundsResumed = true;
    }

    //NOTE: transition done
    if(soundsResumed && soundsMuted) {
        mSectorTransition = false;
        //Logger::get().info("Sector sound trasition end");
    }
}

void Audio::update() {
    if(!mInited) return;

    //NOTE: update listener
    if(auto* scene = App::get()->getScene()) {
        auto* cam = scene->getActiveCamera();
        auto* camSector = scene->getCameraSector();
        
        ALfloat listenerOri[] = { cam->Front.x  * -1.0f, cam->Front.y * -1.0f, cam->Front.z * -1.0f, cam->Up.x, cam->Up.y, cam->Up.z };
        alListener3f(AL_POSITION, cam->Position.x, cam->Position.y, cam->Position.z);
        alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
        alListenerfv(AL_ORIENTATION, listenerOri);
    
        //NOTE: sector sounds transition
        //TODO lerp later
        // if(camSector != mPrevSector) {
        //     mSectorTransition = true;
        //     Logger::get().info("Sector sound trasition start");
        // }

        if(camSector != mCurrentSector) {
            mPrevSector = mCurrentSector;
            mCurrentSector = camSector;
            mSectorTransition = true;
            //Logger::get().info("Sector sound trasition start");
        }

        if(mSectorTransition) {
            doSectorTransition();
        }
    }
}

void Audio::soundOpen(Sound* sound) {
    auto file = Vfs::getFile("sounds\\" + sound->mFile);
    if(!file.has_value()) {
        Logger::get().warn("unable to open sound: {}", sound->mFile);
        return;
    }

    auto& buffer = file.value();
    
    WavHeader wafHeader{};
    buffer.read((char*)&wafHeader, sizeof(WavHeader));

    ALsizei wavBufferSize = sizeof(char) * wafHeader.Subchunk2Size;
    char* wavBuffer = (char *)malloc(wavBufferSize); 
    buffer.read((char*)wavBuffer, wavBufferSize);

    //NOTE: create source and set values
    alGenSources((ALuint)1,  (ALuint*)&sound->mSourceHandle);
    
    //NOTE: update initialy sound
    soundUpdate(sound);

    //NOTE: create buffer and play
    alGenBuffers(1, (ALuint*)&sound->mBufferHandle);
	alBufferData((ALuint)sound->mBufferHandle, wafHeader.NumOfChan == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
			(ALvoid*)wavBuffer, wavBufferSize, wafHeader.SamplesPerSec);
    alSourcei((ALuint)sound->mSourceHandle, AL_BUFFER, (ALuint)sound->mBufferHandle);
}

void Audio::soundUpdate(Sound* sound) {
    const auto& pos = sound->getAbsPos();
    ALuint source = (ALuint)sound->mSourceHandle;
    alSourcef(source,   AL_PITCH,       sound->mPitch);
    alSourcef(source,   AL_GAIN,        sound->mVolume);
    alSourcei(source,   AL_LOOPING,     sound->mIsLooping);
    alSource3f(source,  AL_POSITION,    pos.x, pos.y, pos.z);
    // alSourcef(source,   AL_CONE_INNER_ANGLE,  sound->mCone.x);
    // alSourcef(source,   AL_CONE_OUTER_ANGLE,  sound->mCone.y);
    alSourcef(source,   AL_MAX_DISTANCE, sound->mRadius.OuterRadius);
    alSourcef(source,   AL_ROLLOFF_FACTOR, sound->mRadius.OuterFalloff);
    alSource3f(source,  AL_VELOCITY,    0.0f, 0.0f, 0.0f);
}

void Audio::soundDestroy(Sound* sound) {
    alSourceStop((ALuint)sound->mSourceHandle);
    alDeleteSources(1, (const ALuint*)&sound->mSourceHandle);
    alDeleteBuffers(1, (const ALuint*)&sound->mBufferHandle);
}

void Audio::soundStop(Sound* sound) {
    alSourceStop((ALuint)sound->mSourceHandle);
}

void Audio::soundPause(Sound* sound) {
    alSourcePause((ALuint)sound->mSourceHandle);
}

void Audio::soundPlay(Sound* sound) {
    alSourcePlay((ALuint)sound->mSourceHandle);
}