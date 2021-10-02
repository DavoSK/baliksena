#include "bmp_loader.hpp"

#define SOKOL_IMPL
#define SOKOL_D3D11
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_app.h>
#include <sokol/sokol_glue.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "shader_basic.h"
namespace cutout {

#include "shader_cutout.h"

};
#include "renderer.hpp"

static struct {
    sg_shader shader;
    sg_pipeline pip;

    sg_shader cutoutShader;
    sg_pipeline cutoutPip;

    sg_bindings bind;
    sg_pass_action passAction;
    vs_params_t vsParams;
    RenderPass currentRenderPass;

    RendererMaterial material;
} state;

void Renderer::init() {
    sg_desc desc = { .context = sapp_sgcontext() };
    sg_setup(&desc);

    /* create layoout for pipeline */
    sg_layout_desc layoutDesc {};
    layoutDesc.attrs[ATTR_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3;
    layoutDesc.attrs[ATTR_vs_aNormal].format = SG_VERTEXFORMAT_FLOAT3;
    layoutDesc.attrs[ATTR_vs_aTexCoord].format = SG_VERTEXFORMAT_FLOAT2;

    /* depth buffer stuff */
    sg_depth_state depthState{};
    depthState.compare = SG_COMPAREFUNC_LESS_EQUAL;
    depthState.write_enabled = true;

    //NOTE: basic shader
    {
        /* create shader from code-generated sg_shader_desc */
        state.shader = sg_make_shader(simple_shader_desc(sg_query_backend()));

        /* create pipeline */
        sg_pipeline_desc pipelineDesc = {};
        pipelineDesc.shader         = state.shader;
        pipelineDesc.layout         = layoutDesc;
        pipelineDesc.depth          = depthState;
        pipelineDesc.index_type     = sg_index_type::SG_INDEXTYPE_UINT32;
        pipelineDesc.cull_mode      = sg_cull_mode::SG_CULLMODE_NONE;
        pipelineDesc.label          = "diffuse-pipeline";
        state.pip = sg_make_pipeline(&pipelineDesc);
    }

    //NOTE: cutout shader
    {
        /* create shader from code-generated sg_shader_desc */
        state.cutoutShader = sg_make_shader(cutout::cutout_shader_desc(sg_query_backend()));

        /* create pipeline */
        sg_pipeline_desc pipelineDesc = {};
        pipelineDesc.shader         = state.cutoutShader;
        pipelineDesc.layout         = layoutDesc;
        pipelineDesc.depth          = depthState;
        pipelineDesc.index_type     = sg_index_type::SG_INDEXTYPE_UINT32;
        pipelineDesc.cull_mode      = sg_cull_mode::SG_CULLMODE_NONE;
        pipelineDesc.label          = "cutout-pipeline";
        state.pip = sg_make_pipeline(&pipelineDesc);
    }

    /* a pass action to clear framebuffer */
    sg_color_attachment_action& color = state.passAction.colors[0];
    color.action = SG_ACTION_CLEAR;
    color.value= { 0.2f, 0.3f, 0.3f, 1.0f };
}

void Renderer::destroy() {
    sg_destroy_pipeline(state.pip);
    sg_destroy_shader(state.shader);

    sg_destroy_pipeline(state.cutoutPip);
    sg_destroy_shader(state.cutoutShader);

    sg_shutdown();
}

void Renderer::begin(RenderPass pass) {
    state.currentRenderPass = pass;

    switch(pass) {
        case RenderPass::NORMAL: {
            sg_begin_default_pass(&state.passAction, sapp_width(), sapp_height());
        } break;

        default:
            break;
    }
}

void Renderer::end()  {
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
    state.bind.fs_images[slot] = textureToBind;
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
    state.bind.vertex_buffers[0] = bufferToBind;
}

void Renderer::setIndexBuffer(BufferHandle handle) {
    assert(handle.id != SG_INVALID_ID);
    sg_buffer bufferToBind = { handle.id };
    state.bind.index_buffer = bufferToBind;    
}

void Renderer::bindBuffers() {
    if(state.material.hasTransparencyKey) {
        sg_apply_pipeline(state.cutoutPip);
    } else {
        sg_apply_pipeline(state.pip);
    }

    sg_apply_bindings(&state.bind);
}

void Renderer::setModel(const glm::mat4& model) {
    state.vsParams.model = model;
}

void Renderer::applyUniforms() {
    sg_range uniformsRange{
          &state.vsParams,
          sizeof(vs_params_t)
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &uniformsRange);
}

void Renderer::setViewMatrix(const glm::mat4 &view) {
    state.vsParams.view = view;
}

void Renderer::setProjMatrix(const glm::mat4 &proj) {
    state.vsParams.projection = proj;
}

void Renderer::draw(int baseElement, int numElements, int numInstances) {
    sg_draw(baseElement, numElements, numInstances);
}

int Renderer::getWidth() { return sapp_width(); }
int Renderer::getHeight() { return sapp_height(); }