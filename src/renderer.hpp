#pragma once
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
    glm::vec3 p;
    glm::vec3 n;
    glm::vec2 uv;
};

struct TextureHandle {
    uint32_t id;
};

struct BufferHandle {
    uint32_t id;
};

enum class RenderPass {
    NORMAL,
    TRANSPARENCY,
    CAMERA_RELATIVE,
};

class Renderer {
public:
    static void destroy();
    static void init();
    static void render();
    static void begin(RenderPass pass);
    static void end();
    static void commit();

    static TextureHandle createTexture(uint8_t* data, int width, int height);
    static void destroyTexture(TextureHandle textureHandle);
    static void bindTexture(TextureHandle textureHandle, unsigned int slot);

    static BufferHandle createVertexBuffer(const std::vector<Vertex>& vertices);
    static BufferHandle createIndexBuffer(const std::vector<uint16_t>& indices);
    static void destroyBuffer(BufferHandle bufferHandle);

    static void setViewMatrix(const glm::mat4& view);
    static void setProjMatrix(const glm::mat4& proj);
    static int getWidth();
    static int getHeight();
};