#include "model.hpp"

#include <functional>
#include <algorithm>
#include <memory>
#include <unordered_map>

void Model::render() {
    //if(!isVisible()) return;
    Renderer::setModel(getMatrix());
    Frame::render();
}

void Model::init() {    

}