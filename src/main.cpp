#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_app.h>
#include <sokol/sokol_glue.h>

sg_pass_action pass_action = {};

void init(void) {
    sg_desc desc = {};
    desc.context = sapp_sgcontext();
    sg_setup(&desc);
    pass_action.colors[0] = { SG_ACTION_CLEAR, {1.0f, 0.0f, 0.0f, 1.0f} };
}

void frame(void) {
    float g = pass_action.colors[0].value.g + 0.01f;
    pass_action.colors[0].value.g = (g > 1.0f) ? 0.0f : g;
    sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
    sg_end_pass();
    sg_commit();
}

void cleanup(void) {
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    sapp_desc desc = {};
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.cleanup_cb = cleanup;
    desc.width = 800;
    desc.height = 600;
    desc.gl_force_gles2 = true;
    desc.window_title = "Clear (sokol app)";
    desc.icon.sokol_default = true;
    return desc;
}