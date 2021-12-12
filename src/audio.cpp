#include "audio.hpp"
#include "logger.hpp"

#include "app.hpp"
#include "scene.hpp"
#include "camera.h"
#include "sound.hpp"
#include "vfs.hpp"
#include "sector.hpp"

#include <AL/al.h>
#include <AL/alc.h>


void Audio::init() {
    mDevice = alcOpenDevice(nullptr);
    if (!mDevice) {
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

void Audio::update() {
    if(!mInited) return;

    //NOTE: update listener
    if(auto* scene = App::get()->getScene()) {
        auto* cam = scene->getActiveCamera();
        auto* camSector = scene->getCameraSector();
        
        ALfloat listenerOri[] = { cam->Right.x, cam->Right.y, cam->Right.z, cam->Up.x, cam->Up.y, cam->Up.z };
        alListener3f(AL_POSITION, cam->Position.x, cam->Position.y, cam->Position.z);
        alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
        alListenerfv(AL_ORIENTATION, listenerOri);
    
        //NOTE: sector sounds transition
        //TODO lerp later
        if(camSector != mPrevSector) {
            if(mPrevSector != nullptr) {
                for(auto sound : mPrevSector->getSounds()) {
                    alSourcePause((ALuint)sound->mSourceHandle);
                }
            }

            if(camSector != nullptr) {
                for(auto sound : camSector->getSounds()) {
                    alSourcePlay((ALuint)sound->mSourceHandle);
                }
            }

            mPrevSector = camSector;
        }
    }
}

void Audio::open(Sound* sound) {
    auto file = Vfs::getFile("sounds\\" + sound->mFile);
    if(!file.has_value()) {
        Logger::get().warn("unable to open sound: {}", sound->mFile);
        return;
    }

    /*Read WAV header*/
    struct
    {
        /* RIFF Chunk Descriptor */
        uint8_t         RIFF[4];        // RIFF Header Magic header
        uint32_t        ChunkSize;      // RIFF Chunk Size
        uint8_t         WAVE[4];        // WAVE Header
        /* "fmt" sub-chunk */
        uint8_t         fmt[4];         // FMT header
        uint32_t        Subchunk1Size;  // Size of the fmt chunk
        uint16_t        AudioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
        uint16_t        NumOfChan;      // Number of channels 1=Mono 2=Sterio
        uint32_t        SamplesPerSec;  // Sampling Frequency in Hz
        uint32_t        bytesPerSec;    // bytes per second
        uint16_t        blockAlign;     // 2=16-bit mono, 4=16-bit stereo
        uint16_t        bitsPerSample;  // Number of bits per sample
        /* "data" sub-chunk */
        uint8_t         Subchunk2ID[4]; // "data"  string
        uint32_t        Subchunk2Size;  // Sampled data length
    } WavHeader;

    file.value().read((char*)&WavHeader, sizeof(WavHeader));
    ALsizei wavBufferSize = sizeof(char) * WavHeader.Subchunk2Size;
    char* wavBuffer = (char *)malloc(wavBufferSize); 
    file.value().read((char*)wavBuffer, wavBufferSize);

    //NOTE: create source and set values
    const auto& pos = sound->getAbsPos();
    alGenSources((ALuint)1,  (ALuint*)&sound->mSourceHandle);
    ALuint source = (ALuint)sound->mSourceHandle;

    alSourcef(source,   AL_PITCH,       sound->mPitch);
    alSourcef(source,   AL_GAIN,        sound->mVolume);
    alSourcei(source,   AL_LOOPING,     sound->mIsLooping);
    alSource3f(source,  AL_POSITION,    pos.x, pos.y, pos.z);

    alSourcef(source,  AL_CONE_INNER_ANGLE,  sound->mCone.x);
    alSourcef(source,  AL_CONE_OUTER_ANGLE,  sound->mCone.y);

    alSource3f(source,  AL_VELOCITY,    0.0f, 0.0f, 0.0f);
    
    if(sound->mSoundType == SoundType::Ambient || WavHeader.NumOfChan == 2) {
        alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(source,  AL_POSITION,  0.0f, 0.0f, 0.0f);
        alSourcef(source,  AL_CONE_INNER_ANGLE,  0.0f);
        alSourcef(source,  AL_CONE_OUTER_ANGLE,  0.0f);
    }

    //NOTE: create buffer and play
    alGenBuffers(1, (ALuint*)&sound->mBufferHandle);
	alBufferData((ALuint)sound->mBufferHandle, WavHeader.NumOfChan == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
			(ALvoid*)wavBuffer, wavBufferSize, WavHeader.SamplesPerSec);
    alSourcei(source, AL_BUFFER, (ALuint)sound->mBufferHandle);
	//alSourcePlay(source);
}

void Audio::update(Sound* sound) {

}