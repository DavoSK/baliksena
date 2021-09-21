#pragma once
#include <glm/glm.hpp>

struct RendererCamera {
    glm::mat4 view;
    glm::mat4 proj;
};

class Renderer {
public:
    static Renderer* get() {
        static Renderer* gRenderer = nullptr;
        if(gRenderer == nullptr) 
            gRenderer = new Renderer();
        return gRenderer;
    }

    static void destroy();
    void init();
    void render(const RendererCamera& renderCam);

    int getWidth() const;
    int getHeight() const;
};