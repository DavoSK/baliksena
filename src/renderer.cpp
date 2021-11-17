#include "bmp_loader.hpp"
#include "logger.hpp"

#define SOKOL_LOG(X) Logger::get().warn(X);
#define SOKOL_IMPL

#define SOKOL_D3D11
//#define SOKOL_GLCORE33
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
#include "shader_universal.h"
#include "shader_debug.h"

#include "renderer.hpp"
#include "gui.hpp"

/* debug rendering */
struct Sphere {
    glm::vec3 center;
    float radius;
};

struct Box {
    glm::vec3 center;
    glm::vec3 scale;
};

static struct {
    struct {
        sg_pass_action passAction;
    } display;

    //NOTE: we have array of 2 piplines
    //for backface culling and double sided materials
    struct {
        sg_shader shader;
        sg_pipeline pip[2];

        //sg_shader alphablendShader;
        sg_pipeline alphaPip[2];

        sg_shader debugShader;
        sg_pipeline debugPip;
        sg_bindings bindings;

        sg_pass pass;
        sg_pass_desc passDesc;
        sg_pass_action passAction;
        sg_image colorImg;
    }  offscreen;

    sg_image emptyTexture;

    bool isRelative;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 viewPos;

    Renderer::Material material;
    Renderer::RenderPass pass;
    std::vector<Renderer::Light> lights;

    /* debug */
    sg_buffer debugCubeVertexBuffer;
    sg_buffer debugCubeIndexBuffer;

    std::vector<Box> debugBoxes;
    std::vector<Sphere> debugSphers;
    glm::vec3 debugColor;
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

    /* create empty texture */
    sg_image_desc imageDesc {};
    imageDesc.width         = 1;
    imageDesc.height        = 1;
    imageDesc.pixel_format  = SG_PIXELFORMAT_RGBA8;
    imageDesc.wrap_u        = SG_WRAP_REPEAT;
    imageDesc.wrap_v        = SG_WRAP_REPEAT;
    imageDesc.min_filter    = SG_FILTER_LINEAR;
    imageDesc.mag_filter    = SG_FILTER_LINEAR;

    uint32_t pixel = 0xFFFFFFFF;
    imageDesc.data = { (void*)&pixel, static_cast<size_t>(4) };
    state.emptyTexture = sg_make_image(&imageDesc);

    /* depth buffer stuff */
    sg_depth_state depthState{};
    depthState.compare = SG_COMPAREFUNC_LESS_EQUAL;
    depthState.write_enabled = true;
    depthState.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL;

    /* create layout for ofscreen pipelines */
    sg_layout_desc layoutDesc{};
    layoutDesc.attrs[ATTR_universal_vs_aPos].format         = SG_VERTEXFORMAT_FLOAT3;
    layoutDesc.attrs[ATTR_universal_vs_aNormal].format      = SG_VERTEXFORMAT_FLOAT3;
    layoutDesc.attrs[ATTR_universal_vs_aTexCoord].format    = SG_VERTEXFORMAT_FLOAT2;

    constexpr int OFFSCREEN_SAMPLE_COUNT = 4;

    //NOTE: universal shader
    {
        state.offscreen.shader          = sg_make_shader(universal_universal_shader_desc(sg_query_backend()));

        //NOTE: default pipeline
        {
            sg_pipeline_desc pipelineDesc   = {};
            pipelineDesc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8;
            pipelineDesc.sample_count       = OFFSCREEN_SAMPLE_COUNT;
            pipelineDesc.shader             = state.offscreen.shader;
            pipelineDesc.layout             = layoutDesc;
            pipelineDesc.depth              = depthState;
            pipelineDesc.index_type         = sg_index_type::SG_INDEXTYPE_UINT32;

            pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_NONE;
            pipelineDesc.label              = "universal-pipeline";
            state.offscreen.pip[0]          = sg_make_pipeline(&pipelineDesc);

            pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_BACK;
            pipelineDesc.label              = "universal-pipeline-backfaceculled";
            state.offscreen.pip[1]          = sg_make_pipeline(&pipelineDesc);
        }

        //NOTE: alpha blending pipeline
        {
            sg_pipeline_desc pipelineDesc   = {};
            sg_color_state& colorState      = pipelineDesc.colors[0];
            colorState.pixel_format         = SG_PIXELFORMAT_RGBA8;
            colorState.blend.enabled            = true;
            colorState.blend.src_factor_rgb     = SG_BLENDFACTOR_SRC_ALPHA;
            colorState.blend.src_factor_rgb     = SG_BLENDFACTOR_SRC_ALPHA;
            colorState.blend.dst_factor_rgb     = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            colorState.blend.op_rgb             = SG_BLENDOP_ADD;
            colorState.blend.src_factor_alpha   = SG_BLENDFACTOR_SRC_ALPHA;
            colorState.blend.dst_factor_alpha   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            colorState.blend.op_alpha           = SG_BLENDOP_ADD;

            pipelineDesc.sample_count       = OFFSCREEN_SAMPLE_COUNT;
            pipelineDesc.shader             = state.offscreen.shader;
            pipelineDesc.layout             = layoutDesc;
            pipelineDesc.depth              = depthState;
            pipelineDesc.index_type         = sg_index_type::SG_INDEXTYPE_UINT32;

            pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_NONE;
            pipelineDesc.label              = "universal-pipeline-alphablend";
            state.offscreen.alphaPip[0]     = sg_make_pipeline(&pipelineDesc);

            pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_BACK;
            pipelineDesc.label              = "universal-pipeline-alphablend-backfaceculled";
            state.offscreen.alphaPip[1]     = sg_make_pipeline(&pipelineDesc);
        }
    }

    //NOTE: debug shader
    {
        sg_layout_desc layoutDesc{};
        layoutDesc.attrs[ATTR_debug_vs_aPos].format         = SG_VERTEXFORMAT_FLOAT3;
        state.offscreen.debugShader = sg_make_shader(debug_debug_shader_desc(sg_query_backend()));

        sg_pipeline_desc pipelineDesc   = {};
        pipelineDesc.colors[0].pixel_format = SG_PIXELFORMAT_RGBA8;
        pipelineDesc.sample_count       = OFFSCREEN_SAMPLE_COUNT;
        pipelineDesc.shader             = state.offscreen.debugShader;
        pipelineDesc.layout             = layoutDesc;
        pipelineDesc.depth              = depthState;
        pipelineDesc.index_type         = sg_index_type::SG_INDEXTYPE_UINT16;
        pipelineDesc.cull_mode          = sg_cull_mode::SG_CULLMODE_NONE;
        pipelineDesc.label              = "debug-pipeline";
        state.offscreen.debugPip        = sg_make_pipeline(&pipelineDesc);
    }

    debugInit();
    Gui::init();
}

void Renderer::destroy() {
    sg_destroy_image(state.emptyTexture);
    sg_destroy_shader(state.offscreen.shader);

    for(size_t i = 0; i < 2; i++) {
        sg_destroy_pipeline(state.offscreen.pip[i]);
        sg_destroy_pipeline(state.offscreen.alphaPip[i]);
    }

    /* debug */
    sg_destroy_shader(state.offscreen.debugShader);
    sg_destroy_pipeline(state.offscreen.debugPip);
    sg_shutdown();
}

void Renderer::begin() {
    sg_begin_pass(state.offscreen.pass, &state.offscreen.passAction);
}

void Renderer::setPass(RenderPass pass) {
    state.pass = pass;
    memset(&state.offscreen.bindings, 0, sizeof(sg_bindings));
}

Renderer::RenderPass Renderer::getPass() { return state.pass; }

void Renderer::end()  { sg_end_pass(); }

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

Renderer::TextureHandle Renderer::createTexture(uint8_t* data, int width, int height, bool mipmaps) {
    sg_image_desc imageDesc {};
    imageDesc.width             = width;
    imageDesc.height            = height;
    imageDesc.pixel_format      = SG_PIXELFORMAT_RGBA8;
    imageDesc.wrap_u            = SG_WRAP_REPEAT;
    imageDesc.wrap_v            = SG_WRAP_REPEAT;
    imageDesc.min_filter        = mipmaps ? SG_FILTER_LINEAR_MIPMAP_LINEAR : SG_FILTER_LINEAR;
    imageDesc.mag_filter        = SG_FILTER_LINEAR;
    imageDesc.autogen_mipmaps   = mipmaps;
    imageDesc.data.subimage[0][0] = { data, static_cast<size_t>(width * height * 4) };
    
    sg_image createdImage {SG_INVALID_ID};
    createdImage = sg_make_image(&imageDesc);
    assert(createdImage.id != SG_INVALID_ID);
    return { createdImage.id };
}

void Renderer::destroyTexture(TextureHandle textureHandle) {
    sg_destroy_image({ textureHandle.id });
}

void Renderer::bindTexture(TextureHandle textureHandle, unsigned int slot) {
    sg_image textureToBind = { textureHandle.id };
    state.offscreen.bindings.fs_images[slot] = textureToBind;
}

void Renderer::bindMaterial(const Material& material) {
    state.material = material;
    
    if(material.diffuseTexture.has_value()) {
        bindTexture(material.diffuseTexture.value(), SLOT_universal_diffuseSampler);
    } else {
        bindTexture({state.emptyTexture.id}, SLOT_universal_diffuseSampler);
    }

    if(material.alphaTexture.has_value()) {
        bindTexture(material.alphaTexture.value(), SLOT_universal_alphaSampler);
    } else {
        bindTexture({state.emptyTexture.id}, SLOT_universal_alphaSampler);
    }

    if (material.envTexture.has_value()) {
        bindTexture(material.envTexture.value(), SLOT_universal_envSampler);
    } else {
        bindTexture({state.emptyTexture.id}, SLOT_universal_envSampler);
    }
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
    state.offscreen.bindings.vertex_buffers[0] = bufferToBind;
}

void Renderer::setIndexBuffer(BufferHandle handle) {
    assert(handle.id != SG_INVALID_ID);
    sg_buffer bufferToBind = { handle.id };
    state.offscreen.bindings.index_buffer = bufferToBind;    
}

void Renderer::bindBuffers() {
    if(state.pass ==Renderer::RenderPass::DEBUG) {
        sg_apply_pipeline(state.offscreen.debugPip);
    } else if (state.pass == Renderer::RenderPass::ALPHA) {
        sg_apply_pipeline(state.offscreen.alphaPip[state.material.isDoubleSided ? 0 : 1]);
    } else {
        sg_apply_pipeline(state.offscreen.pip[state.material.isDoubleSided ? 0 : 1]);
    }

    sg_apply_bindings(&state.offscreen.bindings);
}

void Renderer::setModel(const glm::mat4& model) {
    state.model = model;
}

void Renderer::setLights(const std::vector<Light>& lights) {
    state.lights = lights;
}

void Renderer::applyUniforms() {
    if(state.pass == Renderer::RenderPass::DEBUG) {
        debug_vs_params_t vertexUniforms{
            state.model,
            state.view,
            state.proj,
            state.debugColor
        };

        sg_range uniformsRange{
            &vertexUniforms,
            sizeof(debug_vs_params_t)
        };

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_debug_vs_params, &uniformsRange);
        return;
    }

    //NOTE: apply vertex stage uniforms
    {
        //NOTE since sokol uses only floating point uniforms
        //we need to stick with that :/
        const auto isBillboard = state.material.kind == MaterialKind::BILLBOARD ? 1.0f : 0.0f;
        universal_vs_params_t vertexUniforms{};
        vertexUniforms.model = state.model;
        vertexUniforms.view = state.view;
        vertexUniforms.projection = state.proj;
        vertexUniforms.viewPos = state.viewPos;
        vertexUniforms.billboard = isBillboard;
        vertexUniforms.relative = state.isRelative ? 1.0f : 0.0f;
        vertexUniforms.lightsCount = (float)state.lights.size();
        sg_range uniformsRange{
            &vertexUniforms,
            sizeof(universal_vs_params_t)
        };

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_universal_vs_params, &uniformsRange);

        //NOTE: apply material
        universal_vs_material_t vsMaterial{};
        vsMaterial.ambient      = glm::vec4(state.material.ambient, 1.0f);
        vsMaterial.diffuse      = glm::vec4(state.material.diffuse, 1.0f);
        vsMaterial.emissive     = glm::vec4(state.material.emission, 1.0f);

        sg_range vsMaterialRange{
            &vsMaterial,
            sizeof(universal_vs_material_t)
        };
        
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_universal_vs_material, &vsMaterialRange);

        //NOTE: apply lights uniform
        universal_vs_lights_t vsLights = {};
        memset(&vsLights, 0, sizeof(universal_vs_lights_t));

        for(size_t i = 0; i < MaxLights; i++) {
            if( i >= state.lights.size()) break;
            const auto& light = state.lights[i];
            vsLights.position[i]    = glm::vec4(light.position, (float)light.type);
            vsLights.dir[i]         = glm::vec4(light.dir, 0.0f);
            vsLights.ambient[i]     = glm::vec4(light.ambient, 0.0f);
            vsLights.diffuse[i]     = glm::vec4(light.diffuse, 0.0f);
            vsLights.range[i]       = glm::vec4(light.range.x, light.range.y, 0.0f, 0.0f);
            vsLights.cone[i]        = glm::vec4(light.cone.x, light.cone.y, 0.0f, 0.0f);
        }

        sg_range vsLightRange{
            &vsLights,
            sizeof(universal_vs_lights_t)
        };
        
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_universal_vs_lights, &vsLightRange);
    }

    //NOTE: apply fragment state uniforms
    {
        universal_fs_params_t fsUniforms{};
        fsUniforms.envMode = state.material.envTexture.has_value() ? static_cast<float>(state.material.envTextureBlending) : 3.0f;
        fsUniforms.envRatio = state.material.envTextureBlendingRatio;
        sg_range fsUniformsRange{
            &fsUniforms,
            sizeof(universal_fs_params_t)
        };

        sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_universal_fs_params, &fsUniformsRange);
    }
}

void Renderer::setViewMatrix(const glm::mat4 &view) {
    state.view = view;
}

void Renderer::setProjMatrix(const glm::mat4 &proj) {
    state.proj = proj;
}

void Renderer::setViewPos(const glm::vec3& pos) {
    state.viewPos = pos;
}

void Renderer::setCamRelative(bool relative) {
    state.isRelative = relative;
}

bool Renderer::isCamRelative() {
    return state.isRelative;
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

/* debug */
void Renderer::debugSetRenderColor(const glm::vec3& color) {
    state.debugColor = color;
}

void Renderer::debugRenderBox(const glm::vec3& center, const glm::vec3& scale) {
    //state.debugBoxes.push_back({center, scale});

    state.offscreen.bindings.index_buffer = state.debugCubeIndexBuffer;
    state.offscreen.bindings.vertex_buffers[0] = state.debugCubeVertexBuffer;

    auto cubeBox = glm::translate(glm::mat4(1), center) * glm::scale(glm::mat4(1), scale);
    setModel(cubeBox);
    bindBuffers();
    applyUniforms();
    draw(0, 36, 1);
}

void Renderer::debugRenderSphere(const glm::vec3& center, float radius) {
    state.debugSphers.push_back({center, radius});
}

void Renderer::debugInit() {
    //NOTE: init vertex buffer for simple cube rendering
    {
        float vertices[] = {
            // Front face
            -1.0, -1.0,  1.0,
            1.0, -1.0,  1.0,
            1.0,  1.0,  1.0,
            -1.0,  1.0,  1.0,

            // Back face
            -1.0, -1.0, -1.0,
            -1.0,  1.0, -1.0,
            1.0,  1.0, -1.0,
            1.0, -1.0, -1.0,

            // Top face
            -1.0,  1.0, -1.0,
            -1.0,  1.0,  1.0,
            1.0,  1.0,  1.0,
            1.0,  1.0, -1.0,

            // Bottom face
            -1.0, -1.0, -1.0,
            1.0, -1.0, -1.0,
            1.0, -1.0,  1.0,
            -1.0, -1.0,  1.0,

            // Right face
            1.0, -1.0, -1.0,
            1.0,  1.0, -1.0,
            1.0,  1.0,  1.0,
            1.0, -1.0,  1.0,

            // Left face
            -1.0, -1.0, -1.0,
            -1.0, -1.0,  1.0,
            -1.0,  1.0,  1.0,
            -1.0,  1.0, -1.0,
        };

        sg_buffer_desc bufferDesc  = {};
        bufferDesc.data            = SG_RANGE(vertices);
        bufferDesc.label           = "debug-cube-vertex-buffer";

        state.debugCubeVertexBuffer = sg_make_buffer(&bufferDesc);
        assert(state.debugCubeVertexBuffer.id != SG_INVALID_ID);
    }

    //NOTE: debug cube index buffer
    {
        uint16_t indices[] = {
            0,  1,  2,      0,  2,  3,    // front
            4,  5,  6,      4,  6,  7,    // back
            8,  9,  10,     8,  10, 11,   // top
            12, 13, 14,     12, 14, 15,   // bottom
            16, 17, 18,     16, 18, 19,   // right
            20, 21, 22,     20, 22, 23,   // left
        };

        sg_buffer_desc bufferDesc = {};
        bufferDesc.data     = SG_RANGE(indices);
        bufferDesc.type     = sg_buffer_type::SG_BUFFERTYPE_INDEXBUFFER;
        bufferDesc.label    = "debug-cube-index-buffer";

        state.debugCubeIndexBuffer = sg_make_buffer(&bufferDesc);
        assert(state.debugCubeIndexBuffer.id != SG_INVALID_ID);
    }
}

void Renderer::debugBegin() {

}

void Renderer::debugEnd() {

}