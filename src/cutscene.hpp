#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include "mafia/rep.hpp"

#include "frame.hpp"

class Cutscene {
public:
    void start(const std::string& file);
    void tick();

    [[nodiscard]] bool isPlaying() const { return mIsPlaying; }
    void setPlaying(bool play) { mIsPlaying = play; }
private:
    void updateCamera();
    void updateObjects();
    std::unordered_map<uint32_t, std::shared_ptr<Frame>> mDefinitions;
    uint64_t mStartTime = 0;
    bool mIsPlaying = false;
    RepFile::File mRep;
};