#include "sound.hpp"
#include "audio.hpp"
#include "app.hpp"
#include "scene.hpp"

Sound::~Sound() {
    App::get()->getAudio()->soundDestroy(this);
}

bool Sound::open(const std::string& file) {
    mFile = file;
    App::get()->getAudio()->soundOpen(this);
    return true;
}

void Sound::render() {
    Frame::render();

    if(mWasChanged) {
        App::get()->getAudio()->soundUpdate(this);
        mWasChanged = false;
    }
}
void Sound::play() {
    App::get()->getAudio()->soundPlay(this);
    mIsPlaying = true;
}

void Sound::pause() {
    App::get()->getAudio()->soundPause(this);
    mIsPlaying = false;
}

