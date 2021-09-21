#define SOKOL_IMPL
#define SOKOL_D3D11
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_app.h>
#include <sokol/sokol_glue.h>

#include <flecs/flecs.h>

#include <components/input.hpp>
#include <components/camera.hpp>
#include <systems/camera.h>
#include <iostream>
#include <unordered_map>

flecs::world ecs;
sg_pass_action pass_action = {};

void sokolInit(void) {
    setvbuf (stdout, NULL, _IONBF, 0);
    sg_desc desc = { .context = sapp_sgcontext() };
    sg_setup(&desc);

    pass_action.colors[0] = { SG_ACTION_CLEAR, {1.0f, 0.0f, 0.0f, 1.0f} };
    CameraSystem::add(ecs);

    AllocConsole();
    freopen("CONOUT$", "w", stdout);

    auto mainCameraEntity = ecs.entity("Main camera")
            .set<Camera>({  .MovementSpeed = 10.0f, .MouseSensitivity = 0.1f})
            .set<Input>({});

    std::cout << mainCameraEntity.id() << std::endl;
}

void sokolFrame(void) {
    /*float g = pass_action.colors[0].value.g + 0.01f;
    pass_action.colors[0].value.g = (g > 1.0f) ? 0.0f : g;
    sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
    sg_end_pass();
    sg_commit();*/
    ecs.progress();
}

void sokolCleanup(void) {
    sg_shutdown();
}

void sokolEvent(const sapp_event* event) {
    static std::unordered_map<sapp_keycode, bool> keysState;

    ecs.query<Input>().each([event](flecs::entity e, Input& p) {

        const auto getMovementVec = [p, event]() {
            float x = keysState[SAPP_KEYCODE_W] ? 1.0f : keysState[SAPP_KEYCODE_S] ? -1.0f : 0.0f;
            float y = keysState[SAPP_KEYCODE_SPACE] ? 1.0f : keysState[SAPP_KEYCODE_LEFT_SHIFT] ? -1.0f : 0.0f;
            float z = keysState[SAPP_KEYCODE_A] ? 1.0f : keysState[SAPP_KEYCODE_D] ? -1.0f : 0.0f;
            return glm::vec3(x, y, z);
        };

        switch (event->type) {
            case sapp_event_type::SAPP_EVENTTYPE_KEY_DOWN: {
                keysState[event->key_code] = true;
            } break;

            case sapp_event_type::SAPP_EVENTTYPE_KEY_UP: {
                keysState[event->key_code] = false;
            } break;

            case sapp_event_type::SAPP_EVENTTYPE_MOUSE_MOVE: {
                p.mouseDeltaX = event->mouse_dx;
                p.mouseDeltaY = event->mouse_dy;
                p.mouseX = event->mouse_x;
                p.mouseY = event->mouse_y;
            } break;

            default:
                break;
        }

        p.movementDir = getMovementVec();
    });
}

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    sapp_desc desc{};
    desc.init_cb            = sokolInit;
    desc.frame_cb           = sokolFrame;
    desc.cleanup_cb         = sokolCleanup;
    desc.event_cb           = sokolEvent;
    desc.width              = 800;
    desc.height             = 600;
    desc.window_title       = "Senko";
    desc.gl_force_gles2     = true;
    desc.icon.sokol_default = true;
    return desc;
}