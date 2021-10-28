#include <sokol/sokol_app.h>
#include "app.hpp"

void init(void)                 { App::get()->init(); }
void frame(void)                { App::get()->render(); }
void cleanup(void)              { App::get()->destroy(); }
void event(const sapp_event* e) { App::get()->event(e); }

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    sapp_desc desc{};
    desc.init_cb            = init;
    desc.frame_cb           = frame;
    desc.cleanup_cb         = cleanup;
    desc.event_cb           = event;
    desc.width              = 1600;
    desc.height             = 900;
    desc.swap_interval      = 1;
    desc.sample_count       = 4;
    desc.window_title       = "Senko";
    desc.gl_force_gles2     = true;
    desc.enable_clipboard   = true;
    desc.icon.sokol_default = false;
    return desc;
}