#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <optional>

#include "frustum_culling.h"
struct sapp_event;

class Renderer {
public:
    enum class RenderPass {
        NORMAL,
        SKYBOX,
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

    static constexpr int InvalidHandle = 0;
    struct TextureHandle { uint32_t id; };  
    struct BufferHandle { uint32_t id; };
    struct Material {
        std::optional<TextureHandle> diffuseTexture;
        std::optional<TextureHandle> alphaTexture;
        std::optional<TextureHandle> envTexture;
        TextureBlending envTextureBlending;
        float envTextureBlendingRatio;
        float transparency;
        glm::vec3 ambient = {1.0f, 1.0f, 1.0f};
        glm::vec3 diffuse = {1.0f, 1.0f, 1.0f};
        glm::vec3 emission = {1.0f, 1.0f, 1.0f};
        MaterialKind kind;
        bool isDoubleSided;
        bool isColored;
        bool hasTransparencyKey;
    };

    enum class LightType { Dir, Point, Ambient, Spot };
    struct Light {
        LightType type;
        glm::vec3 position;
        glm::vec3 dir;
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec2 range;
        glm::vec2 cone;
    };

    struct Vertex {
        glm::vec3 p;
        glm::vec3 n;
        glm::vec2 uv;
    };

    static void destroy();
    static void init();
    static void begin();
    static void setPass(RenderPass pass);
    static RenderPass getPass();
    static void end();
    static void commit();

    static TextureHandle createTexture(uint8_t* data, int width, int height, bool mipmaps = false);
    static void destroyTexture(TextureHandle textureHandle);
    static void bindTexture(TextureHandle textureHandle, unsigned int slot);
    static void bindMaterial(const Material& material);

    static BufferHandle createVertexBuffer(const std::vector<Vertex>& vertices);
    static BufferHandle createIndexBuffer(const std::vector<uint32_t>& indices);
    static void destroyBuffer(BufferHandle bufferHandle);
    static void setVertexBuffer(BufferHandle handle);
    static void setIndexBuffer(BufferHandle handle);
    static void bindBuffers();

    static void setModel(const glm::mat4& model);
    static void setLights(const std::vector<Light>& light);
    static void applyUniforms();

    static void setViewMatrix(const glm::mat4& view);
    static void setProjMatrix(const glm::mat4& proj);
    static void setViewPos(const glm::vec3& pos);

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