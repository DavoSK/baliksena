#include "gui.hpp"
#include "frame.hpp"
#include "light.hpp"
#include "sector.hpp"
#include "app.hpp"
#include "scene.hpp"
#include "camera.h"
#include "stats.hpp"
#include "vfs.hpp"
#include "logger.hpp"

#include "imgui_ansi.hpp"
#include "IconsFontAwesome5.h"

#include "ImGuizmo.h"
#include "ImSequencer.h"
#include "ImZoomSlider.h"
#include "ImCurveEdit.h"
#include "GraphEditor.h"

#include <string>
#include <vector>

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace ImGui
{
    static auto vector_getter = [](void* vec, int idx, const char** out_text)
    {
        auto& vector = *static_cast<std::vector<std::string>*>(vec);
        if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
        *out_text = vector.at(idx).c_str();
        return true;
    };

    bool Combo(const char* label, int* currIndex, std::vector<std::string>& values)
    {
        if (values.empty()) { return false; }
        return Combo(label, currIndex, vector_getter,
            static_cast<void*>(&values), values.size());
    }

    bool ListBox(const char* label, int* currIndex, std::vector<std::string>& values)
    {
        if (values.empty()) { return false; }
        return ListBox(label, currIndex, vector_getter,
            static_cast<void*>(&values), values.size());
    }
};

static char gNodeSearchText[32] = {};
static size_t gNodeSearchTexLen = 0;
static std::unordered_map<uint32_t, std::vector<std::string>> gLogBuffer;
static Frame* gSelectedNode = nullptr;
static int gButtonIdCnt = 0;
static ImVec2 lastWsize;

struct {
    ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE mCurrentGizmoMode = ImGuizmo::WORLD;

    bool useSnap = false;
    float snap[3] = { 1.f, 1.f, 1.f };
    float bounds[6] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
    float boundsSnap[3] = { 0.1f, 0.1f, 0.1f };
    bool boundSizing = false;
    bool boundSizingSnap = false;
} gizmo;

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

void renderLightWidget(Light* light) {
    ImGui::Separator();
    glm::vec3 diffuse = light->getDiffuse();
    if(ImGui::ColorPicker3("Diffuse", (float*)&diffuse)) {
        light->setDiffuse(diffuse);
    }

    ImGui::Separator();

    switch(light->getType()){
        case LightType::Dir: {
            glm::vec3 dir = light->getDir();
            if(ImGui::InputFloat3("Dir", (float*)&dir)) {
                light->setDir(dir);
            }
        } break;
        case LightType::Point: {
            glm::vec3 pos = light->getPos();
            if(ImGui::InputFloat3("Pos", (float*)&pos)) {
                light->setPos(pos);
            }
        } break;
        default:
            break;
    }
}

void renderSectorWidget(Sector* sector) {
    ImGui::Separator();
    ImGui::Text("Sector: %s", sector->getName().c_str());
    // ImGui::Text("Sector lights:");
    // for(const auto& sectorLight: sector->getLights()) {
    //     ImGui::Text(sectorLight->getName().c_str());
    // }
}

void renderInspectWidget(Frame* frame) {
    if(frame == nullptr)
        return;

    ImGui::Text("Frame name: %s", frame->getName().c_str());
    ImGui::Text("Frame owner: %s", frame->getOwner() != nullptr ? frame->getOwner()->getName().c_str() : nullptr);
    ImGui::Text("Frame type: %d", frame->getFrameType());

    bool isFrameOn = frame->isOn();
    if(ImGui::Checkbox("Frame ON", &isFrameOn)) {
        frame->setOn(isFrameOn);
    }
    
    ImGui::Separator();

    //NOTE: render guizmo controlls
    auto frameMatrix = frame->getMatrix();
    if (ImGui::RadioButton("Translate",  gizmo.mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
         gizmo.mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", gizmo.mCurrentGizmoOperation == ImGuizmo::ROTATE))
         gizmo.mCurrentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale",  gizmo.mCurrentGizmoOperation == ImGuizmo::SCALE))
         gizmo.mCurrentGizmoOperation = ImGuizmo::SCALE;

    float matrixTranslation[3], matrixRotation[3], matrixScale[3];
    ImGuizmo::DecomposeMatrixToComponents(reinterpret_cast<const float*>(&frameMatrix), matrixTranslation, matrixRotation, matrixScale);
    ImGui::InputFloat3("Transl", matrixTranslation);
    ImGui::InputFloat3("Rotation", matrixRotation);
    ImGui::InputFloat3("Scale", matrixScale);
    
    //NOTE: teleport button
    if(ImGui::Button("Teleport")) {
        if(auto* cam = App::get()->getScene()->getActiveCamera()) {
            auto worldMat = gSelectedNode->getWorldMatrix();
            cam->Position = glm::vec3(worldMat[3]);
        }
    }

    switch(gSelectedNode->getFrameType()) {
        case FrameType::Light: {
            renderLightWidget(dynamic_cast<Light*>(gSelectedNode));
        } break;

        case FrameType::Sector: {
            renderSectorWidget(dynamic_cast<Sector*>(gSelectedNode));
        } break;

        default: 
            break;
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

    ImGui::BeginChild("##scrolling");
    for(const auto& msg : gLogBuffer[currentLogLevel]) {
        ImGui::TextAnsiUnformatted(msg.c_str(), nullptr);
    }
    ImGui::SetScrollHereY();
    ImGui::EndChild();
}

void renderSceneWidget(Scene* scene) {
    auto& missionList = Vfs::getMissionsList();
    static int currentMissionIdx = 49;  
    ImGui::Combo("##missionscombo", &currentMissionIdx, missionList); 

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_COMPACT_DISC)) {
        if (!missionList[currentMissionIdx].empty()) {
            scene->load(missionList[currentMissionIdx]);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_FA_TRASH)) {
        gSelectedNode = nullptr;
        scene->clear();
    }

    if (ImGui::InputText(ICON_FA_SEARCH, gNodeSearchText, 32)) {
        gNodeSearchTexLen = strnlen(gNodeSearchText, 32);
    }

    ImGui::Separator();

    gButtonIdCnt = 0;
    renderNodeRecursively(scene);
}

void editTransform(float* cameraView, float* cameraProjection, float* matrix, float* deltaMat) {
    ImGuizmo::SetDrawlist();
    auto windowWidth  = (float)ImGui::GetWindowWidth();
    auto windowHeight = (float)ImGui::GetWindowHeight();
    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);
    ImGuizmo::Manipulate(cameraView, 
        cameraProjection, 
        gizmo.mCurrentGizmoOperation, 
        gizmo.mCurrentGizmoMode, 
        matrix, 
        deltaMat, 
        gizmo.useSnap ? &gizmo.snap[0] : nullptr, 
        gizmo.boundSizing ? gizmo.bounds : nullptr, 
        gizmo.boundSizingSnap ? gizmo.boundsSnap : nullptr
    );
}

glm::vec2 getNormalizedCoords() {
    ImGuiIO& io = ImGui::GetIO();
    auto windowPos = ImGui::GetWindowPos();
    auto windowSize = ImGui::GetWindowSize();
    float x = -((windowSize.x - (io.MousePos.x - windowPos.x)) / windowSize.x - 0.5f) * 2.0f;
    float y = ((windowSize.y - (io.MousePos.y - windowPos.y)) / windowSize.y - 0.5f) * 2.0f;
    return {x, y};
}

glm::vec4 toEyeCoords(glm::vec4 clipCoords, const glm::mat4& projection) {
    glm::vec4 invertedProjection = glm::inverse(projection) * clipCoords;
    return glm::vec4(invertedProjection.x, invertedProjection.y, 1.0f, 0.0f);
}

glm::vec3 toWorldCoords(glm::vec4 eyeCoords, const glm::mat4& view) {
    glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * eyeCoords);
    glm::vec3 mouseRay = glm::vec3(rayWorld.x, rayWorld.y, rayWorld.z);
    return glm::normalize(rayWorld);
}

glm::vec3 createRay(Camera* cam) {
    auto normalizedCoords = getNormalizedCoords();
    glm::vec4 clipCoords = glm::vec4(normalizedCoords.x, normalizedCoords.y, -1.0f, 1.0f);
    glm::vec4 eyeCoords = toEyeCoords(clipCoords, cam->getProjMatrix());
    glm::vec3 worldRay = toWorldCoords(eyeCoords, cam->getViewMatrix());
    return worldRay;
}

bool wasClickedInViewport() {
    ImGuiIO& io = ImGui::GetIO();
    const auto windowPos = ImGui::GetWindowPos();
    const auto windowSize = ImGui::GetWindowSize();
    const auto x = io.MousePos.x - windowPos.x;
    const auto y = io.MousePos.y - windowPos.y;
    return ImGui::IsMouseClicked(ImGuiMouseButton_Left, true) && 
           !ImGuizmo::IsUsing() && 
           !ImGuizmo::IsOver() && ( x >= 0.0f && x <= windowSize.x && 
        y >= 0.0f && y <= windowSize.y);
}

void getNearesetFrame(const glm::vec3& pos, Frame* node, float* dist, std::optional<Frame*>& foundFrame) {    
    if (!node) return;

    const auto isPointInsideAABB = [](const glm::vec3& point, const glm::vec3& min, const glm::vec3& max) -> bool {
        return (point.x >= min.x && point.x <= max.x) && (point.y >= min.y && point.y <= max.y) && (point.z >= min.z && point.z <= max.z);
    };

    // const auto& wmat = node->getWorldBBOX();
    // if(glm::length(wmat.first) > 0.0f && 
    //    glm::length(wmat.second) > 0.0f && isPointInsideAABB(pos, wmat.first, wmat.second)) {
        
    //     glm::vec3 center = glm::vec3((wmat.first.x + wmat.second.x) / 2.0f, ( wmat.first.y + wmat.second.y) /2, ( wmat.first.z + wmat.second.z ) / 2);
    //     float distLen = glm::length(center - pos);
    //     if(distLen < *dist) {
    //         *dist = distLen;
    //         foundFrame = node;
    //     }
    // }

    const auto& wmat = node->getWorldMatrix();
    const auto meshPos = glm::vec3(wmat[3]);
    if(glm::length(meshPos) > 0.0f) {
        const auto distLen = glm::length(meshPos - pos);
        if(distLen < *dist) {
            *dist = distLen;
            foundFrame = node;
        }
    }

    for (const auto& node : node->getChilds()) {
        getNearesetFrame(pos, node.get(), dist, foundFrame);
    }
};

void renderImGuizmo(Scene* scene) {
    if(auto* cam = scene->getActiveCamera()) {
        //NOTE: do raycast for frame picking
        if(wasClickedInViewport()) {
            constexpr float toAdd  = 0.15f;
            auto rayDir = createRay(cam);
            auto rayStart = cam->Position;

            float someDistance = toAdd;
            std::optional<Frame*> pickedFrame;
            float dist = std::numeric_limits<float>::max();
            for(size_t i = 0; i < 1000; i++) {
                glm::vec3 currentPoint = rayStart + (rayDir * someDistance);
                getNearesetFrame(currentPoint, scene, &dist, pickedFrame);
                 someDistance += toAdd;
            }

            if(pickedFrame.has_value()) {
                gSelectedNode = pickedFrame.value();
            }
        }

        if(gSelectedNode == nullptr) return;
        glm::mat4 view = cam->getViewMatrix();
        glm::mat4 proj = cam->getProjMatrix();
        glm::mat4 nodeWorld = gSelectedNode->getWorldMatrix();
        glm::mat4 deltaMat = glm::mat4(1.0f);
        editTransform(reinterpret_cast<float*>(&view), 
                        reinterpret_cast<float*>(&proj), 
                        reinterpret_cast<float*>(&nodeWorld), 
                        reinterpret_cast<float*>(&deltaMat));
        if(ImGuizmo::IsUsing()) {
            gSelectedNode->setMatrix(gSelectedNode->getMatrix() * deltaMat);
        }
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
    renderSceneWidget(scene);
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
    renderImGuizmo(scene);
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
    auto sector = scene->getCameraSector();
    if(sector != nullptr) {
        ImGui::Text("Camera sector: %s", sector->getName().c_str());
        // ImGui::Text("Sector lights:");
        // for(const auto& sectorLight: sector->getLights()) {
        //     ImGui::Text(sectorLight->getName().c_str());
        // }
    }


    ImGui::Separator();
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

void Gui::addLogMessage(uint32_t logLevel, const std::string& message) {
    gLogBuffer[logLevel].push_back(message);
    gLogBuffer[LOGGER_ALL_INDEX].push_back(message);
}
