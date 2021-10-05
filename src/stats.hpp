#pragma once
struct Stats {
    unsigned int framesInUse;
    unsigned int billboardsInUse;
    unsigned int modelsInUse;
    unsigned int texturesInUse;
};

extern Stats gStats;