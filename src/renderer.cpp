#include "bmp_loader.hpp"
#include "logger.hpp"

#define SOKOL_LOG(X) Logger::get().warn(X);
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
#include "shader_env.h"
#include "shader_skybox.h"

#include "renderer.hpp"
#include "gui.hpp"

static struct {
    struct {
        sg_pass_action passAction;
    } display;

    //NOTE: we have array of 2 piplines
    //for backface culling and double sided materials
    struct {
        sg_shader shader;
        sg_pipeline pip[2];

        sg_shader cutoutShader;
        sg_pipeline cutoutPip[2];

        sg_shader envShader;
        sg_pipeline envPip[2];

        sg_shader billboardShader;
        sg_pipeline billboardPip[2];

        //NOTE: skybox will have backface culling always on
        //why not ?
        sg_shader skyboxShader;
        sg_pipeline skyboxPip;
        

        sg_bindings bind;
        sg_pass pass;
        sg_pass_desc passDesc;
        sg_pass_action passAction;
        sg_image colorImg;
    }  offscreen;

    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 viewPos;
    Renderer::Material material;
    Renderer::RenderPass pass;
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

        pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_NONE;
        pipelineDesc.label              = "diffuse-pipeline";
        state.offscreen.pip[0]          = sg_make_pipeline(&pipelineDesc);

        pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_BACK;
        pipelineDesc.label              = "diffuse-pipeline-backfaceculled";
        state.offscreen.pip[1]          = sg_make_pipeline(&pipelineDesc);
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

        pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_NONE;
        pipelineDesc.label              = "cutout-pipeline";
        state.offscreen.cutoutPip[0]    = sg_make_pipeline(&pipelineDesc);

        pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_BACK;
        pipelineDesc.label              = "cutout-pipeline-backfaceculled";
        state.offscreen.cutoutPip[1]    = sg_make_pipeline(&pipelineDesc);
    }

    //NOTE: env shader
    {
        state.offscreen.envShader       = sg_make_shader(env_env_shader_desc(sg_query_backend()));

        sg_layout_desc envLayoutDesc{};
        envLayoutDesc.attrs[ATTR_env_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3;
        envLayoutDesc.attrs[ATTR_env_vs_aNormal].format = SG_VERTEXFORMAT_FLOAT3;
        envLayoutDesc.attrs[ATTR_env_vs_aTexCoord].format = SG_VERTEXFORMAT_FLOAT2;

        sg_pipeline_desc pipelineDesc   = {};
        pipelineDesc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8;
        pipelineDesc.sample_count       = OFFSCREEN_SAMPLE_COUNT;
        pipelineDesc.shader             = state.offscreen.envShader;
        pipelineDesc.layout             = envLayoutDesc;
        pipelineDesc.depth              = depthState;
        pipelineDesc.index_type         = sg_index_type::SG_INDEXTYPE_UINT32;

        pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_NONE;
        pipelineDesc.label              = "env-pipeline";
        state.offscreen.envPip[0]       = sg_make_pipeline(&pipelineDesc);

        pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_BACK;
        pipelineDesc.label              = "env-pipeline-backfaceculled";
        state.offscreen.envPip[1]       = sg_make_pipeline(&pipelineDesc);
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

        pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_NONE;
        pipelineDesc.label              = "billboard-pipeline";
        state.offscreen.billboardPip[0] = sg_make_pipeline(&pipelineDesc);

        pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_BACK;
        pipelineDesc.label              = "billboard-pipeline-backfaceculled";
        state.offscreen.billboardPip[1] = sg_make_pipeline(&pipelineDesc);
    }

    //NOTE: skybox shader
    {
        state.offscreen.skyboxShader = sg_make_shader(skybox_skybox_shader_desc(sg_query_backend()));

        sg_pipeline_desc pipelineDesc   = {};
        pipelineDesc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8;
        pipelineDesc.sample_count       = OFFSCREEN_SAMPLE_COUNT;
        pipelineDesc.shader             = state.offscreen.skyboxShader;
        pipelineDesc.layout             = layoutDesc;
        pipelineDesc.depth              = depthState;
        pipelineDesc.index_type         = sg_index_type::SG_INDEXTYPE_UINT32;
        pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_NONE;
        pipelineDesc.label              = "skybox-pipeline";
        state.offscreen.skyboxPip       = sg_make_pipeline(&pipelineDesc);
    }

    Gui::init();
}

void Renderer::destroy() {
    sg_destroy_shader(state.offscreen.shader);
    sg_destroy_shader(state.offscreen.cutoutShader);
    sg_destroy_shader(state.offscreen.envShader);
    sg_destroy_shader(state.offscreen.billboardShader);

    for(size_t i = 0; i < 2; i++) {
        sg_destroy_pipeline(state.offscreen.pip[i]);
        sg_destroy_pipeline(state.offscreen.cutoutPip[i]);
        sg_destroy_pipeline(state.offscreen.envPip[i]);
        sg_destroy_pipeline(state.offscreen.billboardPip[i]);
    }

    sg_destroy_shader(state.offscreen.skyboxShader);
    sg_destroy_pipeline(state.offscreen.skyboxPip);

    sg_shutdown();
}

void Renderer::begin() {
    sg_begin_pass(state.offscreen.pass, &state.offscreen.passAction);
}

void Renderer::setPass(RenderPass pass) {
    state.pass = pass;
}

Renderer::RenderPass Renderer::getPass() {
    return state.pass;
}

void Renderer::end()  {
    sg_end_pass();
}

void Renderer::commit() { 
    //NOTE: begin default pass for GUI
    {
        static uint64_t lastTime = 0;
        const auto width = sapp_width();
        const auto height = sapp_height();

        const auto deltaTime = stm_sec(stm_laptime(&lastTime));
        simgui_new_frame(width, height, deltaTime);
        sg_begin_default_pass(&state.display.passAction, width, height);
    
        Gui::render();
        simgui_render();
        sg_end_pass();
    }

    sg_commit(); 
}

Renderer::TextureHandle Renderer::createTexture(uint8_t* data, int width, int height) {
    sg_image_desc imageDesc {};
    imageDesc.width         = width;
    imageDesc.height        = height;
    imageDesc.pixel_format  = SG_PIXELFORMAT_RGBA8;
    imageDesc.wrap_u        = SG_WRAP_REPEAT;
    imageDesc.wrap_v        = SG_WRAP_REPEAT;
    imageDesc.min_filter    = SG_FILTER_LINEAR;
    imageDesc.mag_filter    = SG_FILTER_LINEAR;
    imageDesc.data = { data, static_cast<size_t>(width * height * 4) };

    sg_image createdImage = sg_make_image(&imageDesc);
    assert(createdImage.id != SG_INVALID_ID);
    return { createdImage.id };
}

void Renderer::destroyTexture(TextureHandle textureHandle) {
    sg_destroy_image({ textureHandle.id });
}

void Renderer::bindTexture(TextureHandle textureHandle, unsigned int slot) {
    sg_image textureToBind = { textureHandle.id };
    state.offscreen.bind.fs_images[slot] = textureToBind;
}

void Renderer::bindMaterial(const Material& material) {
    if(material.diffuseTexture.has_value()) {
        bindTexture(material.diffuseTexture.value(), SLOT_basic_texture1);
    }

    if (material.envTexture.has_value()) {
        bindTexture(material.envTexture.value(), SLOT_env_envSampler);
    } else {
        bindTexture({SG_INVALID_ID}, SLOT_env_envSampler);
    }

    state.material = material;
}

Renderer::BufferHandle Renderer::createVertexBuffer(const std::vector<Vertex>& vertices) {
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

Renderer::BufferHandle Renderer::createIndexBuffer(const std::vector<uint32_t>& indices) {
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
    if(state.pass == Renderer::RenderPass::SKYBOX) {
        sg_apply_pipeline(state.offscreen.skyboxPip);
    } else {
        //NOTE: get index in our pipeline array based if its boudle sided material or not
        size_t piplineIdx = state.material.isDoubleSided ? 0 : 1;
        switch (state.material.kind) {
            case MaterialKind::BILLBOARD: {
                sg_apply_pipeline(state.offscreen.billboardPip[piplineIdx]);
            } break;

            case MaterialKind::ENV: {
                sg_apply_pipeline(state.offscreen.envPip[piplineIdx]);
            } break;

            case MaterialKind::CUTOUT: {
                sg_apply_pipeline(state.offscreen.cutoutPip[piplineIdx]);
            } break;

            default: {
                sg_apply_pipeline(state.offscreen.pip[piplineIdx]);
            } break;
        }
    }

    sg_apply_bindings(&state.offscreen.bind);
}

void Renderer::setModel(const glm::mat4& model) {
    state.model = model;
}

void Renderer::applyUniforms() {

    if(state.pass == Renderer::RenderPass::SKYBOX) {
        skybox_vs_params_t vertexUniforms{
            state.model,
            state.view,
            state.proj
        };

        sg_range uniformsRange{
            &vertexUniforms,
            sizeof(skybox_vs_params_t)
        };

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_skybox_vs_params, &uniformsRange);
        return;
    }

    switch(state.material.kind) {
        case MaterialKind::BILLBOARD: {
            //NOTE: apply vertex stage uniforms
            {
                billboard_vs_params_t vsUniforms {
                    state.model,
                    state.view,
                    state.proj
                };

                sg_range vsUniformsRange{
                    &vsUniforms,
                    sizeof(billboard_vs_params_t)
                };

                sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_basic_vs_params, &vsUniformsRange);
            }
        } break;

        case MaterialKind::ENV: {
            //NOTE: apply vertex stage uniforms
            {
                env_vs_params_t vsUniforms{
                    state.model,
                    state.view,
                    state.proj,
                    state.viewPos
                };

                sg_range vsUniformsRange{
                    &vsUniforms,
                    sizeof(env_vs_params_t)
                };

                sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_basic_vs_params, &vsUniformsRange);
            }

            //NOTE: apply fragment state uniforms
            {
                env_fs_params_t fsUniforms{
                    static_cast<float>(state.material.envTextureBlending),
                    state.material.envTextureBlendingRatio
                };

                sg_range fsUniformsRange{
                   &fsUniforms,
                   sizeof(env_fs_params_t)
                };

                sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_env_fs_params, &fsUniformsRange);
            }
        } break;

        default: {
            basic_vs_params_t vertexUniforms{
                 state.model,
                 state.view,
                 state.proj
            };

            sg_range uniformsRange{
                &vertexUniforms,
                sizeof(basic_vs_params_t)
            };

            sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_basic_vs_params, &uniformsRange);
        } break;
    }
}

void Renderer::setViewMatrix(const glm::mat4 &view) {
    state.view = view;
    updateFrustum();
}

void Renderer::setProjMatrix(const glm::mat4 &proj) {
    state.proj = proj;
    updateFrustum();
}

void Renderer::setViewPos(const glm::vec3& pos)
{
    state.viewPos = pos;
}

void Renderer::draw(int baseElement, int numElements, int numInstances) {
    sg_draw(baseElement, numElements, numInstances);
}

Renderer::TextureHandle Renderer::getRenderTargetTexture() {
    assert(state.offscreen.colorImg.id != SG_INVALID_ID);
    return { state.offscreen.colorImg.id };
}

void Renderer::guiHandleSokolInput(const sapp_event* e) { 
    simgui_handle_event(e); 
}

int Renderer::getWidth() { return sapp_width(); }
int Renderer::getHeight() { return sapp_height(); }

void Renderer::updateFrustum() {
    mFurstum = Frustum(state.proj * state.view);
}

Frustum Renderer::mFurstum;
