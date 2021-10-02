#pragma once
#include "mesh.hpp"

class Billboard : public Mesh {
    public:
        void render() override;
};