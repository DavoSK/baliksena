#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <optional>

#include "frustum_culling.h"
struct sapp_event;

enum class RenderPass {
    NORMAL,
    BILLBOARD,
    TRANSPARENCY,
    CAMERA_RELATIVE,
};

enum class MaterialKind {
    DIFFUSE,
    CUTOUT,
    ALPHA,
    ENV,
    BILLBOARD
};

enum class TextureBlending {
    NORMAL,
    MUL,
    ADD
};

struct Vertex {
    glm::vec3 p;
    glm::vec3 n;
    glm::vec2 uv;
};

struct TextureHandle {
    uint32_t id;
};

struct RendererMaterial {
    std::optional<TextureHandle> diffuseTexture;
    std::optional<TextureHandle> alphaTexture;
    std::optional<TextureHandle> envTexture;
    TextureBlending envTextureBlending;
    float envTextureBlendingRatio;
    bool isDoubleSided;
    bool hasTransparencyKey;
    float transparency;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 emission;
    MaterialKind kind;
};

struct BufferHandle {
    uint32_t id;
};

class Renderer {
public:
    static void destroy();
    static void init();
    static void begin(RenderPass pass);
    static void end();
    static void commit();

    static TextureHandle createTexture(uint8_t* data, int width, int height);
    static void destroyTexture(TextureHandle textureHandle);
    static void bindTexture(TextureHandle textureHandle, unsigned int slot);
    static void bindMaterial(const RendererMaterial& material);

    static BufferHandle createVertexBuffer(const std::vector<Vertex>& vertices);
    static BufferHandle createIndexBuffer(const std::vector<uint32_t>& indices);
    static void destroyBuffer(BufferHandle bufferHandle);
    static void setVertexBuffer(BufferHandle handle);
    static void setIndexBuffer(BufferHandle handle);
    static void bindBuffers();

    static void setModel(const glm::mat4& model);
    static void applyUniforms();

    static void setViewMatrix(const glm::mat4& view);
    static void setProjMatrix(const glm::mat4& proj);
    static Frustum& getFrustum() { return mFurstum; }

    static void draw(int baseElement, int numElements, int numInstances);

    static void createRenderTarget(int width, int height);
    static TextureHandle getRenderTargetTexture();

    static void guiHandleSokolInput(const sapp_event* e);
    static int getWidth();
    static int getHeight();

private: 
    static void updateFrustum();
    static Frustum mFurstum;
};