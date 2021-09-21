#include <flecs/flecs.h>
#include <glm/gtc/matrix_transform.hpp>
#include <components/input.hpp>
#include <components/camera.hpp>
#include <systems/camera.h>

#include <sokol/sokol_app.h>

#include <iostream>
#include <unordered_map>

#include "renderer.hpp"

flecs::world ecs;
flecs::entity cameraEntity;

void sokolInit(void) {
    CameraSystem::add(ecs);

    auto* renderer = Renderer::get();
    renderer->init();
    cameraEntity= ecs.entity("Main camera")
       .set<Camera>({  
           .MovementSpeed = 10.0f, 
           .MouseSensitivity = 0.1f, 
           .ProjMatrix = glm::perspectiveLH(glm::radians(65.0f), (float)renderer->getWidth() / (float)renderer->getHeight(), 0.01f, 2000.0f)})
       .set<Input>({});
}

void sokolFrame(void) {
    auto* camComponent = cameraEntity.get<Camera>();
    RendererCamera rendererCamera = {
        camComponent->ViewMatrix,
        camComponent->ProjMatrix
    };

    Renderer::get()->render(rendererCamera);
    ecs.progress();
}

void sokolCleanup(void) {
    Renderer::destroy();
}

void sokolEvent(const sapp_event* event) {
    static std::unordered_map<sapp_keycode, bool> keysState;

    ecs.query<Input>().each([event](flecs::entity e, Input& p) {
        const auto getMovementVec = [p, event]() -> glm::vec3 { 
            return {
                keysState[SAPP_KEYCODE_W] ?       1.0f : keysState[SAPP_KEYCODE_S] ?          -1.0f : 0.0f,
                keysState[SAPP_KEYCODE_SPACE] ?   1.0f : keysState[SAPP_KEYCODE_LEFT_SHIFT] ? -1.0f : 0.0f,
                keysState[SAPP_KEYCODE_A] ?       1.0f : keysState[SAPP_KEYCODE_D] ?          -1.0f : 0.0f
            };
        };

        switch (event->type) {
            case sapp_event_type::SAPP_EVENTTYPE_KEY_DOWN: {
                keysState[event->key_code] = true;
            } break;

            case sapp_event_type::SAPP_EVENTTYPE_KEY_UP: {
                keysState[event->key_code] = false;
            } break;

            case sapp_event_type::SAPP_EVENTTYPE_MOUSE_MOVE: {
                p.mouseDeltaX = event->mouse_dx;
                p.mouseDeltaY = event->mouse_dy;
                p.mouseX = event->mouse_x;
                p.mouseY = event->mouse_y;
            } break;

            default:
                break;
        }

        p.movementDir = getMovementVec();
    });
}

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    sapp_desc desc{};
    desc.init_cb            = sokolInit;
    desc.frame_cb           = sokolFrame;
    desc.cleanup_cb         = sokolCleanup;
    desc.event_cb           = sokolEvent;
    desc.width              = 800;
    desc.height             = 600;
    desc.window_title       = "Senko";
    desc.gl_force_gles2     = true;
    desc.icon.sokol_default = false;
    return desc;
}