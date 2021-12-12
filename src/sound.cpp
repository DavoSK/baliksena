#include "sound.hpp"
#include "audio.hpp"
#include "app.hpp"
#include "scene.hpp"

bool Sound::open(const std::string& file) {
    mFile = file;
    App::get()->getAudio()->open(this);
    return true;
}

void Sound::render() {
    Frame::render();

    if(mWasChanged) {
        App::get()->getAudio()->update(this);
        mWasChanged = false;
    }
}