#include "input.hpp"

void Input::updateFromSokolEvent(const sapp_event* e) {
    const auto getMovementVec = [this]() -> glm::vec3 { 
        return {
            mKeysState[SAPP_KEYCODE_W] ?       1.0f : mKeysState[SAPP_KEYCODE_S] ?          -1.0f : 0.0f,
            mKeysState[SAPP_KEYCODE_SPACE] ?   1.0f : mKeysState[SAPP_KEYCODE_LEFT_SHIFT] ? -1.0f : 0.0f,
            mKeysState[SAPP_KEYCODE_A] ?       1.0f : mKeysState[SAPP_KEYCODE_D] ?          -1.0f : 0.0f
        };
    };

    switch (e->type) {
        case sapp_event_type::SAPP_EVENTTYPE_KEY_DOWN: {
            mKeysState[e->key_code] = true;
        } break;
        case sapp_event_type::SAPP_EVENTTYPE_KEY_UP: {
            mKeysState[e->key_code] = false;
        } break;
        case sapp_event_type::SAPP_EVENTTYPE_MOUSE_MOVE: {
            mMouseDelta = { e->mouse_dx, e->mouse_dy };
            mMouse = { e->mouse_x, e->mouse_y };
        } break;

        default:
            break;
    }

    mMoveDir = getMovementVec();
}

void Input::clearDeltas() {
    mMouseDelta = {0.0f, 0.0f};
}
