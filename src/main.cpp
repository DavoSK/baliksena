#define SOKOL_IMPL
#define SOKOL_D3D11
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_app.h>
#include <sokol/sokol_glue.h>

#include <flecs/flecs.h>
#include <components/input.hpp>

flecs::world ecs;
sg_pass_action pass_action = {};

void sokolInit(void) {
    sg_desc desc = { .context = sapp_sgcontext() };
    sg_setup(&desc);

    pass_action.colors[0] = { SG_ACTION_CLEAR, {1.0f, 0.0f, 0.0f, 1.0f} };
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
    ecs.each([event](flecs::entity e, Input& p) {
        switch (event->type) {
            case sapp_event_type::SAPP_EVENTTYPE_KEY_DOWN: {
            } break;

            case sapp_event_type::SAPP_EVENTTYPE_KEY_UP: {
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