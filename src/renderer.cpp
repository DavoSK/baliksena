#include "bmp_loader.hpp"
#include "logger.hpp"

#define SOKOL_LOG(X) Logger::get().warn(X);
#define SOKOL_IMPL

//#define SOKOL_D3D11
//#define SOKOL_WGPU
#define SOKOL_GLCORE33
#include <sokol/sokol_time.h>
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_app.h>
#include <sokol/sokol_glue.h>
#include <sokol/sokol_gl.h>

#include "imgui/imgui.h"

#define SOKOL_IMGUI_IMPL
#include <sokol/util/sokol_imgui.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "shader_universal.h"
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
    std::vector<glm::mat4> bones; 
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
    
    /* setup sokol-gl for fast debug rendering */
    sgl_desc_t sgldesc = {};
    sgldesc.sample_count = sapp_sample_count();
    sgl_setup(sgldesc);

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
    layoutDesc.attrs[ATTR_universal_vs_aIndexes].format     = SG_VERTEXFORMAT_FLOAT2;
    layoutDesc.attrs[ATTR_universal_vs_aWeights].format     = SG_VERTEXFORMAT_FLOAT2;

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

    Gui::init();
}

void Renderer::destroy() {
    sg_destroy_image(state.emptyTexture);
    sg_destroy_shader(state.offscreen.shader);

    for(size_t i = 0; i < 2; i++) {
        sg_destroy_pipeline(state.offscreen.pip[i]);
        sg_destroy_pipeline(state.offscreen.alphaPip[i]);
    }

    simgui_shutdown();
    sgl_shutdown();
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

void Renderer::end()  {  sgl_draw(); sg_end_pass(); }

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
    if (state.pass == Renderer::RenderPass::ALPHA) {
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

void Renderer::setBones(const std::vector<glm::mat4>& bones) {
    state.bones = bones;
}

void Renderer::applyUniforms() {
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
        vertexUniforms.bonesCount = (float)state.bones.size();
        
        //NOTE: set bones
        memset(vertexUniforms.bones, 0, sizeof(glm::mat4) * MaxBones);
        for(size_t i = 0; i < state.bones.size(); i++) {
            vertexUniforms.bones[i] = state.bones[i];
        }

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

void GenerateSphereSmooth(float radius, float latitudes, float longitudes, std::vector<glm::vec3>& verts, std::vector<uint16_t>& indices)
{
    #define M_PI 3.14159265358979323846264338327950288

    if(longitudes < 3)
        longitudes = 3;
    
    if(latitudes < 2)
        latitudes = 2;

    float deltaLatitude = M_PI / latitudes;
    float deltaLongitude = 2 * M_PI / longitudes;
    float latitudeAngle;
    float longitudeAngle;

    // Compute all vertices first except normals
    for (int i = 0; i <= latitudes; ++i)
    {
        latitudeAngle = M_PI / 2 - i * deltaLatitude; /* Starting -pi/2 to pi/2 */
        float xy = radius * cosf(latitudeAngle);    /* r * cos(phi) */
        float z = radius * sinf(latitudeAngle);     /* r * sin(phi )*/

        /*
         * We add (latitudes + 1) vertices per longitude because of equator,
         * the North pole and South pole are not counted here, as they overlap.
         * The first and last vertices have same position and normal, but
         * different tex coords.
         */
        for (int j = 0; j <= longitudes; ++j)
        {
            longitudeAngle = j * deltaLongitude;
            verts.push_back({
                xy * cosf(longitudeAngle),       /* x = r * cos(phi) * cos(theta)  */
                xy * sinf(longitudeAngle),       /* y = r * cos(phi) * sin(theta) */
                z                                /* z = r * sin(phi) */
            });

        }
    }

    /*
     *  Indices
     *  k1--k1+1
     *  |  / |
     *  | /  |
     *  k2--k2+1
     */
    unsigned int k1, k2;
    for(int i = 0; i < latitudes; ++i)
    {
        k1 = i * (longitudes + 1);
        k2 = k1 + longitudes + 1;
        // 2 Triangles per latitude block excluding the first and last longitudes blocks
        for(int j = 0; j < longitudes; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (latitudes - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

void Renderer::immBegin() {
    sgl_defaults();
    sgl_matrix_mode_projection();
    sgl_load_matrix((const float*)&state.proj);
    sgl_matrix_mode_modelview();
    sgl_load_matrix((const float*)&state.view);
    sgl_begin_lines();
}

void Renderer::immEnd() {
    sgl_end();
}

void Renderer::immSetColor(const glm::vec3& color) {
    sgl_c3f(color.x, color.y, color.z);
}

void Renderer::immRenderLine(const glm::vec3& p0, const glm::vec3& p1) {
    sgl_v3f(p0.x, p0.y, p0.z);
    sgl_v3f(p1.x, p1.y, p1.z);
}

void Renderer::immRenderSphere(const glm::vec3& pos, float radius) {
    sgl_defaults();
    sgl_matrix_mode_projection();
    sgl_load_matrix((const float*)&state.proj);
    sgl_matrix_mode_modelview();
    sgl_load_matrix((const float*)&state.view);
    sgl_c3f(1.0f, 0.0f, 0.0f);

    const float PI = 3.141592f;
    GLfloat x, y, z, alpha, beta; // Storage for coordinates and angles        
    int gradation = 20;

    for (alpha = 0.0; alpha < PI; alpha += PI/gradation)
    {        
        sgl_begin_triangle_strip();
        for (beta = 0.0; beta < 2.01*PI; beta += PI/gradation)            
        {            
            x = pos.x + radius*cos(beta)*sin(alpha);
            y = pos.y + radius*sin(beta)*sin(alpha);
            z = pos.z + radius*cos(alpha);
            sgl_v3f(x, y, z);

            x = pos.x + radius*cos(beta)*sin(alpha + PI/gradation);
            y = pos.y + radius*sin(beta)*sin(alpha + PI/gradation);
            z = pos.z + radius*cos(alpha + PI/gradation);            
            sgl_v3f(x, y, z);            
        }        
        sgl_end();
    }
}
