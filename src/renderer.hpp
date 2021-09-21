#pragma once
#include <glm/glm.hpp>

struct RendererCamera {
    glm::mat4 view;
    glm::mat4 proj;
};

class Renderer {
public:
    static void destroy();
    static void init();
    static void render(const RendererCamera& renderCam);
    static int getWidth();
    static int getHeight();
};