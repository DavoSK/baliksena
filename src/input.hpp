#pragma once
#include <glm/glm.hpp>
#include <sokol/sokol_app.h>

#include <unordered_map>

class Input {
public:
    Input() {}
    ~Input()  {}
    void updateFromSokolEvent(const sapp_event* e);
    bool isKeyDown(sapp_keycode keyCode) { return mKeysState[keyCode]; }
    const glm::vec2& getMouseDelta() const { return mMouseDelta; }
    const glm::vec3& getMoveDir() const { return mMoveDir; }
private:
    glm::vec2 mMouse = {0.0f, 0.0f};
    glm::vec2 mMouseDelta = {0.0f, 0.0f};
    glm::vec3 mMoveDir = {0.0f, 0.0f, 0.0f};
    std::unordered_map<sapp_keycode, bool> mKeysState;
};