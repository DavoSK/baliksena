#include "bmp_loader.hpp"

#define SOKOL_IMPL
//#define SOKOL_D3D11
#define SOKOL_GLCORE33
#include <sokol/sokol_time.h>
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_app.h>
#include <sokol/sokol_glue.h>

#include "imgui/imgui.h"

#define SOKOL_IMGUI_IMPL
#include <sokol/util/sokol_imgui.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

//NOTE: shaders
#include "shader_basic.h"
#include "shader_cutout.h" 
#include "shader_billboard.h" 


#include "renderer.hpp"
#include "gui.hpp"

static struct {
    struct {
        sg_pass_action passAction;
    } display;

    struct {
        sg_shader shader;
        sg_pipeline pip;

        sg_shader cutoutShader;
        sg_pipeline cutoutPip;

        sg_shader billboardShader;
        sg_pipeline billboardPip;

        sg_bindings bind;
        sg_pass pass;
        sg_pass_desc passDesc;
        sg_pass_action passAction;
        sg_image colorImg;

        basic_vs_params_t vsParams;
    }  offscreen;

    RendererMaterial material;
} state;

void Renderer::createRenderTarget(int width, int height) {
    
    /* destroy previous resource (can be called for invalid id) */
    sg_destroy_pass(state.offscreen.pass);
    sg_destroy_image(state.offscreen.passDesc.color_attachments[0].image);
    sg_destroy_image(state.offscreen.passDesc.depth_stencil_attachment.image);

    /* create offscreen rendertarget images and pass */
    sg_image_desc colorImgDesc      = {};
    colorImgDesc.render_target      = true;
    colorImgDesc.width              = width;
    colorImgDesc.height             = height;
    colorImgDesc.pixel_format       = SG_PIXELFORMAT_RGBA8;
    colorImgDesc.wrap_u             = SG_WRAP_CLAMP_TO_EDGE;
    colorImgDesc.wrap_v             = SG_WRAP_CLAMP_TO_EDGE;
    colorImgDesc.min_filter         = SG_FILTER_LINEAR;
    colorImgDesc.mag_filter         = SG_FILTER_LINEAR;
    colorImgDesc.label              = "color-image";
    state.offscreen.colorImg        = sg_make_image(&colorImgDesc);

    //NOTE: depth
    sg_image_desc depthImgDesc      = colorImgDesc;
    depthImgDesc.pixel_format       = SG_PIXELFORMAT_DEPTH_STENCIL;
    depthImgDesc.label              = "depth-image";
    sg_image depthImg               = sg_make_image(&depthImgDesc);

    state.offscreen.passDesc = {};
    state.offscreen.passDesc.color_attachments[0].image = state.offscreen.colorImg;
    state.offscreen.passDesc.depth_stencil_attachment.image = depthImg;
    state.offscreen.passDesc.label = "offscreen-pass";
    state.offscreen.pass = sg_make_pass(&state.offscreen.passDesc);
}

void Renderer::init() {
    /* init sokol*/
    sg_desc desc = { .context = sapp_sgcontext() };
    sg_setup(&desc);
    stm_setup();

    /* a pass action to clear offscreen framebuffer */
    state.display.passAction = {};
    sg_color_attachment_action& colorDisplay = state.display.passAction.colors[0];
    colorDisplay.action = SG_ACTION_CLEAR;
    colorDisplay.value = { 0.0f, 0.0f, 0.0f, 1.0f };

    state.display.passAction.depth.action       = SG_ACTION_DONTCARE;
    state.display.passAction.stencil.action     = SG_ACTION_DONTCARE;

    /* a pass action to clear offscreen framebuffer */
    sg_color_attachment_action& colorOffscreen = state.offscreen.passAction.colors[0];
    colorOffscreen.action = SG_ACTION_CLEAR;
    colorOffscreen.value = { 1.0f, 1.0f, 1.0f, 1.0f };
    
    /* init gui */
    simgui_desc_t simgui_desc = { };
    simgui_setup(&simgui_desc);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    /* a render pass with one color- and one depth-attachment image */
    createRenderTarget(sapp_width(), sapp_height());

    /* depth buffer stuff */
    sg_depth_state depthState{};
    depthState.compare = SG_COMPAREFUNC_LESS_EQUAL;
    depthState.write_enabled = true;
    depthState.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL;

    /* create layout for ofscreen pipelines */
    sg_layout_desc layoutDesc{};
    layoutDesc.attrs[ATTR_basic_vs_aPos].format         = SG_VERTEXFORMAT_FLOAT3;
    layoutDesc.attrs[ATTR_basic_vs_aNormal].format      = SG_VERTEXFORMAT_FLOAT3;
    layoutDesc.attrs[ATTR_basic_vs_aTexCoord].format    = SG_VERTEXFORMAT_FLOAT2;

    constexpr int OFFSCREEN_SAMPLE_COUNT = 4;

    //NOTE: basic shader
    {
        state.offscreen.shader          = sg_make_shader(basic_simple_shader_desc(sg_query_backend()));

        sg_pipeline_desc pipelineDesc   = {};
        pipelineDesc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8;
        pipelineDesc.sample_count       = OFFSCREEN_SAMPLE_COUNT;
        pipelineDesc.shader             = state.offscreen.shader;
        pipelineDesc.layout             = layoutDesc;
        pipelineDesc.depth              = depthState;
        pipelineDesc.index_type         = sg_index_type::SG_INDEXTYPE_UINT32;
        pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_BACK;
        pipelineDesc.label              = "diffuse-pipeline";
        state.offscreen.pip             = sg_make_pipeline(&pipelineDesc);
    }

    //NOTE: cutout shader
    {
        state.offscreen.cutoutShader    = sg_make_shader(cutout_cutout_shader_desc(sg_query_backend()));

        sg_pipeline_desc pipelineDesc   = {};
        pipelineDesc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8;
        pipelineDesc.sample_count       = OFFSCREEN_SAMPLE_COUNT;
        pipelineDesc.shader             = state.offscreen.cutoutShader;
        pipelineDesc.layout             = layoutDesc;
        pipelineDesc.depth              = depthState;
        pipelineDesc.index_type         = sg_index_type::SG_INDEXTYPE_UINT32;
        pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_BACK;
        pipelineDesc.label              = "cutout-pipeline";
       
        state.offscreen.cutoutPip       = sg_make_pipeline(&pipelineDesc);
    }

    //NOTE: billboard shader
    {
        state.offscreen.billboardShader = sg_make_shader(billboard_billboard_shader_desc(sg_query_backend()));

        sg_pipeline_desc pipelineDesc   = {};
        pipelineDesc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8;
        pipelineDesc.sample_count       = OFFSCREEN_SAMPLE_COUNT;
        pipelineDesc.shader             = state.offscreen.billboardShader;
        pipelineDesc.layout             = layoutDesc;
        pipelineDesc.depth              = depthState;
        pipelineDesc.index_type         = sg_index_type::SG_INDEXTYPE_UINT32;
        pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_BACK;
        pipelineDesc.label              = "billboard-pipeline";
        state.offscreen.billboardPip    = sg_make_pipeline(&pipelineDesc);
    }
}

void Renderer::destroy() {
    sg_destroy_pipeline(state.offscreen.pip);
    sg_destroy_shader(state.offscreen.shader);

    sg_destroy_pipeline(state.offscreen.cutoutPip);
    sg_destroy_shader(state.offscreen.cutoutShader);

    sg_destroy_pipeline(state.offscreen.billboardPip);
    sg_destroy_shader(state.offscreen.billboardShader);

    sg_shutdown();
}

static uint64_t last_time = 0;

void Renderer::begin(RenderPass pass) {
    sg_begin_pass(state.offscreen.pass, &state.offscreen.passAction);
}

void Renderer::end()  {
    sg_end_pass();
    
    const int width = sapp_width();
    const int height = sapp_height();

    const double deltaTime = stm_sec(stm_laptime(&last_time));
    simgui_new_frame(width, height, deltaTime);
    sg_begin_default_pass(&state.display.passAction, sapp_width(), sapp_height());
    Gui::render();
    simgui_render();
    sg_end_pass();
}

void Renderer::commit() { sg_commit(); }

TextureHandle Renderer::createTexture(uint8_t* data, int width, int height) {
    sg_image_desc imageDesc {};
    imageDesc.width         = width;
    imageDesc.height        = height;
    imageDesc.pixel_format  = SG_PIXELFORMAT_RGBA8;
    imageDesc.wrap_u        = SG_WRAP_REPEAT;
    imageDesc.wrap_v        = SG_WRAP_REPEAT;
    imageDesc.min_filter    = SG_FILTER_LINEAR;
    imageDesc.mag_filter    = SG_FILTER_LINEAR;
    imageDesc.data = { data, static_cast<size_t>(width * height * 4) };
    return { sg_make_image(&imageDesc).id };
}

void Renderer::destroyTexture(TextureHandle textureHandle) {
    sg_destroy_image({ textureHandle.id });
}

void Renderer::bindTexture(TextureHandle textureHandle, unsigned int slot) {
    sg_image textureToBind = { textureHandle.id };
    state.offscreen.bind.fs_images[slot] = textureToBind;
}

void Renderer::bindMaterial(const RendererMaterial& material) {
    if(material.diffuseTexture.has_value()) {
        bindTexture(material.diffuseTexture.value(), 0);
    }

    state.material = material;
   /* if(material.alphaTexture.has_value()) {
        bindTexture(material.alphaTexture.value(), 1);
    }   

    if(material.alphaTexture.has_value()) {
        bindTexture(material.alphaTexture.value(), 2);
    }*/
}

BufferHandle Renderer::createVertexBuffer(const std::vector<Vertex>& vertices) {
    sg_range bufferData {
        vertices.data(),
        sizeof(Vertex) * vertices.size()
    };

    sg_buffer_desc bufferDesc  = {};
    bufferDesc.size            = sizeof(Vertex) * vertices.size();
    bufferDesc.data            = bufferData;
    bufferDesc.label           = "vertex-buffer";

    sg_buffer buffer = sg_make_buffer(&bufferDesc);
    assert(buffer.id != SG_INVALID_ID);
    return { buffer.id };
}

BufferHandle Renderer::createIndexBuffer(const std::vector<uint32_t>& indices) {
    sg_range bufferData = {
        indices.data(), 
        sizeof(uint32_t) * indices.size()
    };

    sg_buffer_desc bufferDesc = {};
    bufferDesc.size     = sizeof(uint32_t) * indices.size();
    bufferDesc.data     = bufferData;
    bufferDesc.type     = sg_buffer_type::SG_BUFFERTYPE_INDEXBUFFER;
    bufferDesc.label    = "index-buffer";

    sg_buffer buffer = sg_make_buffer(&bufferDesc);
    assert(buffer.id != SG_INVALID_ID);
    return { buffer.id };
}

void Renderer::destroyBuffer(BufferHandle bufferHandle) {
    assert(bufferHandle.id != SG_INVALID_ID);
    sg_buffer bufferToDestroy = { bufferHandle.id };
    sg_destroy_buffer(bufferToDestroy);
}

void Renderer::setVertexBuffer(BufferHandle handle) {
    assert(handle.id != SG_INVALID_ID);
    sg_buffer bufferToBind = { handle.id };
    state.offscreen.bind.vertex_buffers[0] = bufferToBind;
}

void Renderer::setIndexBuffer(BufferHandle handle) {
    assert(handle.id != SG_INVALID_ID);
    sg_buffer bufferToBind = { handle.id };
    state.offscreen.bind.index_buffer = bufferToBind;    
}

void Renderer::bindBuffers() {
    switch (state.material.kind) {
        case MaterialKind::BILLBOARD: {
            sg_apply_pipeline(state.offscreen.billboardPip);
        } break;

        case MaterialKind::CUTOUT: {
            sg_apply_pipeline(state.offscreen.cutoutPip);
        } break;

        default: {
            sg_apply_pipeline(state.offscreen.pip);
        } break;
    }

    sg_apply_bindings(&state.offscreen.bind);
}

void Renderer::setModel(const glm::mat4& model) {
    state.offscreen.vsParams.model = model;
}

void Renderer::applyUniforms() {
    sg_range uniformsRange{
          &state.offscreen.vsParams,
          sizeof(basic_vs_params_t)
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_basic_vs_params, &uniformsRange);
}

void Renderer::setViewMatrix(const glm::mat4 &view) {
    state.offscreen.vsParams.view = view;
    updateFrustum();
}

void Renderer::setProjMatrix(const glm::mat4 &proj) {
    state.offscreen.vsParams.projection = proj;
    updateFrustum();
}

void Renderer::draw(int baseElement, int numElements, int numInstances) {
    sg_draw(baseElement, numElements, numInstances);
}

TextureHandle Renderer::getRenderTargetTexture() {
    assert(state.offscreen.colorImg.id != SG_INVALID_ID);
    return { state.offscreen.colorImg.id };
}

void Renderer::guiHandleSokolInput(const sapp_event* e) {
    simgui_handle_event(e);
}

int Renderer::getWidth() { return sapp_width(); }
int Renderer::getHeight() { return sapp_height(); }

void Renderer::updateFrustum() {
    mFurstum = Frustum(state.offscreen.vsParams.projection * state.offscreen.vsParams.view);
}

Frustum Renderer::mFurstum;