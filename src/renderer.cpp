#include "bmp_loader.hpp"

#define SOKOL_IMPL
#define SOKOL_D3D11

#include <sokol/sokol_gfx.h>
#include <sokol/sokol_app.h>
#include <sokol/sokol_glue.h>

#include "renderer.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "shader_basic.h"

static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    uint8_t file_buffer[256 * 1024];
    glm::vec3 cube_positions[10];
} state;

void Renderer::init() {
    sapp_lock_mouse(true);

    sg_desc desc = { .context = sapp_sgcontext() };
    sg_setup(&desc);

    //NOTe: cube positions
    state.cube_positions[0] = { 0.0f,  0.0f,  0.0f};
    state.cube_positions[1] = {2.0f,  5.0f, -15.0f};
    state.cube_positions[2] = {-1.5f, -2.2f, -2.5f};
    state.cube_positions[3] = {-3.8f, -2.0f, -12.3f};
    state.cube_positions[4] = { 2.4f, -0.4f, -3.5f};
    state.cube_positions[5] = {-1.7f,  3.0f, -7.5f};
    state.cube_positions[6] = {1.3f, -2.0f, -2.5f};
    state.cube_positions[7] = {1.5f,  2.0f, -2.5f};
    state.cube_positions[8] = {1.5f,  0.2f, -1.5f};
    state.cube_positions[9] = {-1.3f,  1.0f, -1.5f};

    //NOTE: buffers and stuff
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    
    sg_buffer_desc bufferDesc = {
        .size = sizeof(vertices),
        .data = vertices,
        .label = "cube-vertices"
    };

    state.bind.vertex_buffers[0] = sg_make_buffer(&bufferDesc);

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(simple_shader_desc(sg_query_backend()));

    /* create a pipeline object (default render states are fine for triangle) */
    sg_layout_desc layoutDesc {};
    layoutDesc.attrs[ATTR_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    layoutDesc.attrs[ATTR_vs_aTexCoord].format = SG_VERTEXFORMAT_FLOAT2;

    sg_depth_state depthState{
        .compare = SG_COMPAREFUNC_LESS_EQUAL,
        .write_enabled = true,
    };

    sg_pipeline_desc pipelineDesc = {
        .shader = shd,
        .layout = layoutDesc, 
        .depth = depthState, 
        .label = "triangle-pipeline"
    };

    state.pip = sg_make_pipeline(&pipelineDesc);
    
    /* a pass action to clear framebuffer */
    state.pass_action.colors[0] = { SG_ACTION_CLEAR, {0.2f, 0.3f, 0.3f, 1.0f} };

    /* load first texture */
    state.bind.fs_images[SLOT_texture1] = sg_alloc_image();
    sg_image image1 = state.bind.fs_images[SLOT_texture1];

    int w, h;
    uint8_t* pixels = loadBMPEx("C:\\Mafia\\MAPS\\shot275.bmp", &w, &h, false);
    if (pixels) {
        sg_image_desc imageDesc {};
        imageDesc.width         = w;
        imageDesc.height        = h;
        imageDesc.pixel_format  = SG_PIXELFORMAT_RGBA8;
        imageDesc.wrap_u        = SG_WRAP_REPEAT;
        imageDesc.wrap_v        = SG_WRAP_REPEAT;
        imageDesc.min_filter    = SG_FILTER_LINEAR;
        imageDesc.mag_filter    = SG_FILTER_LINEAR;
        imageDesc.data = {
            pixels, 
            static_cast<size_t>(w * h * 4)
        };

        sg_init_image(image1, imageDesc);
        free(pixels);
    }
}

void Renderer::destroy() {
    sg_shutdown();
}

void Renderer::render(const RendererCamera& renderCam) {        
    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    vs_params_t vs_params = {
        .view = renderCam.view,
        .projection = renderCam.proj
    };

    for(size_t i = 0; i < 10; i++) {
        glm::mat4 model = glm::translate(glm::mat4(1.0), state.cube_positions[i]);
        vs_params.model = model;

        sg_range uniformsRange{ 
            &vs_params, 
            sizeof(vs_params)
        };  

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &uniformsRange);
        sg_draw(0, 36, 1);
    }

    sg_end_pass();
    sg_commit();
}

int Renderer::getWidth() {
    return sapp_width();
}

int Renderer::getHeight() {
    return sapp_height();
}