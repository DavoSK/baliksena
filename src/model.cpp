#include "model.hpp"

#include <functional>
#include <algorithm>
#include <memory>
#include <unordered_map>

void Model::render() {
    if(!isVisible()) return;
    Renderer::setModel(getMatrix());
    Frame::render();
}

void Model::debugRender() {
    Frame::debugRender();
    // if(mSphereBounding != nullptr) {
    //     Renderer::immRenderSphere(mSphereBounding->center, mSphereBounding->radius);
    // }
}

void Model::init() {    

}