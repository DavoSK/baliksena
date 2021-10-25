#include "gui.hpp"
#include "frame.hpp"
#include "app.hpp"
#include "scene.hpp"
#include "camera.h"
#include "stats.hpp"
#include "imgui_ansi.hpp"
#include "imgui/IconsFontAwesome5.h"

#include <string>
#include <vector>

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

static char gNodeSearchText[32] = {};
static size_t gNodeSearchTexLen = 0;
static std::unordered_map<uint32_t, std::vector<std::string>> gLogBuffer;
static Frame* gSelectedNode = nullptr;
static int gButtonIdCnt = 0;
static ImVec2 lastWsize;

bool doesContainChildNode(Frame* frame, const char* frameName) {
    if (!frame) return false;

    if (strstr(frame->getName().c_str(), frameName)) {
        return true;
    } else {
        for (const auto& child : frame->getChilds()) {
            if (doesContainChildNode(child.get(), frameName))
                return true;
        }
    }

    return false;
}

void renderNodeRecursively(Frame* frame) {
    if (frame == nullptr)
        return;

    if ((gNodeSearchTexLen && doesContainChildNode(frame, gNodeSearchText)) || !gNodeSearchTexLen) {
        ImGui::AlignTextToFramePadding();
        ImGui::PushID(gButtonIdCnt++);
        if (ImGui::Button(ICON_FA_EYE)) {
            gSelectedNode = frame;
        }
        ImGui::PopID();
        ImGui::SameLine();

        if (ImGui::TreeNodeEx(frame->getName().c_str(), ImGuiTreeNodeFlags_AllowItemOverlap)) {
            for (const auto& child : frame->getChilds()) {
                renderNodeRecursively(child.get());
            }
            ImGui::TreePop();
        }
    }
}

void renderInspectWidget(Frame* frame) {
    if(frame == nullptr)
        return;

    ImGui::Text("Frame name: %s", frame->getName().c_str());
    ImGui::Text("Frame owner: %s", frame->getOwner() != nullptr ? frame->getOwner()->getName().c_str() : nullptr);
    ImGui::Text("Frame type: %d", frame->getType());

    static bool isFrameOn = frame->isOn();
    if(ImGui::Checkbox("Frame ON", &isFrameOn)) {
        frame->setOn(isFrameOn);
    }

    ImGui::Separator();

    glm::vec3 origScale;
    glm::quat origRotation;
    glm::vec3 origTranslation;
    glm::vec3 origSkew;
    glm::vec4 origPerspective;
    glm::decompose(frame->getMatrix(), origScale, origRotation, origTranslation, origSkew, origPerspective);

    ImGui::Text("Pos: %f %f %f", origTranslation.x, origTranslation.y, origTranslation.z);
    ImGui::Text("Rot: %f %f %f %f", origRotation.x, origRotation.y, origRotation.z, origRotation.w);
    ImGui::Text("Scale: %f %f %f", origScale.x, origScale.y, origScale.z);

    if(ImGui::Button("Teleport")) {
        if(auto* cam = App::get()->getScene()->getActiveCamera()) {
            cam->Position = origTranslation;
        }
    }
}

void Gui::init() {
}

void renderTerminalWidget() {
    static int currentLogLevel = 7;    
    static const char* logLevelNames[8] = { "Trace", "Debug", "Info", "Warn", "Error", "Critical", "Off", "All" };
    static const char* currentLogLevelName = logLevelNames[LOGGER_ALL_INDEX];

    ImGui::SameLine();
    if (ImGui::BeginCombo("##combo", currentLogLevelName)) {
        for (int n = 0; n < IM_ARRAYSIZE(logLevelNames); n++) {
            bool isSelected = (currentLogLevelName == logLevelNames[n]);
            if (ImGui::Selectable(logLevelNames[n], isSelected)) {
                currentLogLevelName = logLevelNames[n];
                currentLogLevel = n;
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if(ImGui::Button(ICON_FA_TRASH)) {
        gLogBuffer.clear();
    }

    for(const auto& msg : gLogBuffer[currentLogLevel]) {
        ImGui::TextAnsiUnformatted(msg.c_str(), nullptr);
    }
}

void Gui::render() {
    auto* scene = App::get()->getScene();
    auto* cam = scene->getActiveCamera();

    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to
    // not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        static auto first_time = true;
        if (first_time) {
            first_time = false;
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

            auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25, nullptr, &dockspace_id);
            auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.30f, nullptr, &dockspace_id);
            auto dock_id_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);
            auto dock_id_debug = ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Up, 0.30f, nullptr, &dock_id_right);
            auto dock_id_center = ImGui::DockBuilderGetCentralNode(dockspace_id);

            ImGui::DockBuilderDockWindow("Scene", dock_id_left);
            ImGui::DockBuilderDockWindow("Log", dock_id_down);
            ImGui::DockBuilderDockWindow("Inspect", dock_id_right);
            ImGui::DockBuilderDockWindow("Debug", dock_id_debug);
            ImGui::DockBuilderDockWindow("View", dock_id_center->ID);
            ImGui::DockBuilderFinish(dockspace_id);
        }
    }

    ImGui::End();

    // NOTE: scene section
    ImGui::Begin("Scene");
    if (ImGui::InputText("Search", gNodeSearchText, 32)) {
        gNodeSearchTexLen = strnlen(gNodeSearchText, 32);
    }

    gButtonIdCnt = 0;
    renderNodeRecursively(scene);
    ImGui::End();

    // NOTE: terminal
    ImGui::Begin("Log");
    renderTerminalWidget();
    ImGui::End();

    // NOTE: inspect
    ImGui::Begin("Inspect");
    renderInspectWidget(gSelectedNode);
    ImGui::End();

    // NOTE: game view
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("View");
    ImGui::BeginChild("GameRender");

    ImVec2 wsize = ImGui::GetWindowSize();
    if (wsize.x != lastWsize.x || wsize.y != lastWsize.y) {
        const auto width = static_cast<int>(wsize.x);
        const auto height = static_cast<int>(wsize.y);
        Renderer::createRenderTarget(width, height);

        if (auto* cam = scene->getActiveCamera()) {
            cam->createProjMatrix(width, height);
        }

        lastWsize = wsize;
    }

    ImGui::Image((ImTextureID)(Renderer::getRenderTargetTexture().id), wsize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleVar();

    // NOTE: debug, camera settings
    ImGui::Begin("Debug");
    ImGui::Text("Frames in use: %d", gStats.framesInUse);
    ImGui::Text("Models in use: %d", gStats.modelsInUse);
    ImGui::Text("Billboards in use: %d", gStats.billboardsInUse);
    ImGui::Text("Textures in use: %d", gStats.texturesInUse);
    ImGui::Separator();

    ImGui::InputFloat3("Camera pos", (float*)&cam->Position);
    ImGui::Separator();

    // NOTE: scene section
    if (ImGui::Button("Clear Scene")) {
       //gSelectedNode = nullptr;
       scene->clear();
    }

    static char fileBuff[250] = "TUTORIAL";
    ImGui::InputText("###model", fileBuff, 250);
    ImGui::SameLine();
    if (ImGui::Button("Load scene")) {
       if (strlen(fileBuff)) {
           scene->load(fileBuff);
       }
    }

    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

void Gui::addLogMessage(uint32_t logLevel, const std::string& message) {
    gLogBuffer[logLevel].push_back(message);
    gLogBuffer[LOGGER_ALL_INDEX].push_back(message);
}
