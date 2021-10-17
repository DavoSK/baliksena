#include "gui.hpp"
#include "frame.hpp"
#include "app.hpp"
#include "scene.hpp"
#include "camera.h"
#include "logger.hpp"
#include "stats.hpp"

#include "imgui_ansi.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>

#include <string>
#include <vector>

static char gNodeSearchText[32] = {};
static size_t gNodeSearchTexLen = 0;
static std::vector<std::string> gLogBuffer;

static bool wasInited = false;
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
        if (ImGui::TreeNode(frame->getName().c_str())) {
            for (const auto& child : frame->getChilds()) {
                renderNodeRecursively(child.get());
            }

            ImGui::TreePop();
            ImGui::Separator();
        }
    }
}

namespace spdlog::sinks {
    template<typename Mutex>
    class gui_sink final : public base_sink<Mutex> {
    public:
        // Formatting codes
        const string_view_t reset = "\033[m";
        const string_view_t bold = "\033[1m";
        const string_view_t dark = "\033[2m";
        const string_view_t underline = "\033[4m";
        const string_view_t blink = "\033[5m";
        const string_view_t reverse = "\033[7m";
        const string_view_t concealed = "\033[8m";
        const string_view_t clear_line = "\033[K";

        // Foreground colors
        const string_view_t black = "\033[30m";
        const string_view_t red = "\033[31m";
        const string_view_t green = "\033[32m";
        const string_view_t yellow = "\033[33m";
        const string_view_t blue = "\033[34m";
        const string_view_t magenta = "\033[35m";
        const string_view_t cyan = "\033[36m";
        const string_view_t white = "\033[37m";

        /// Background colors
        const string_view_t on_black = "\033[40m";
        const string_view_t on_red = "\033[41m";
        const string_view_t on_green = "\033[42m";
        const string_view_t on_yellow = "\033[43m";
        const string_view_t on_blue = "\033[44m";
        const string_view_t on_magenta = "\033[45m";
        const string_view_t on_cyan = "\033[46m";
        const string_view_t on_white = "\033[47m";

        /// Bold colors
        const string_view_t yellow_bold = "\033[33m\033[1m";
        const string_view_t red_bold = "\033[31m\033[1m";
        const string_view_t bold_on_red = "\033[1m\033[41m";

        gui_sink() {
            colors_[level::trace] = to_string_(white);
            colors_[level::debug] = to_string_(cyan);
            colors_[level::info] = to_string_(green);
            colors_[level::warn] = to_string_(yellow_bold);
            colors_[level::err] = to_string_(red_bold);
            colors_[level::critical] = to_string_(bold_on_red);
            colors_[level::off] = to_string_(reset);
        }
        gui_sink(const gui_sink &) = delete;
        gui_sink &operator=(const gui_sink &) = delete;
    protected:
        void sink_it_(const details::log_msg &msg) override {
            memory_buf_t formatted;
            base_sink<Mutex>::formatter_->format(msg, formatted);
            
            std::string outString;
            outString += std::string(formatted.data(), msg.color_range_start);
            outString += colors_[msg.level];
            outString += std::string(formatted.data() + msg.color_range_start, msg.color_range_end - msg.color_range_start);
            outString += reset;
            outString += std::string(formatted.data() + msg.color_range_end, formatted.size() - msg.color_range_end);

            gLogBuffer.push_back(outString);
        }
        void flush_() override {
            gLogBuffer.clear();
        }

        std::string to_string_(std::string_view view) {
            return std::string(view.data(), view.size());
        }
    private:
        std::array<std::string, level::n_levels> colors_;
    };

    using gui_sink_mt = gui_sink<std::mutex>;
    using gui_sink_st = gui_sink<details::null_mutex>;
}; 

ImVec2 lastWsize;

void Gui::render() {
    auto* scene = App::get()->getScene();
    auto* cam = scene->getActiveCamera();

    if (!wasInited) {
     //   custom_command_struct cmd_struct; // terminal commands can interact with this structure
     //   terminal_log = std::make_unique<ImTerm::terminal<terminal_commands>>(cmd_struct, "Terminal");
     //   terminal_log->set_min_log_level(ImTerm::message::severity::info)
        Logger::get().sinks().push_back(std::make_shared<spdlog::sinks::gui_sink_st>());
        wasInited = true;
    }

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

            ImGui::DockBuilderRemoveNode(dockspace_id);  // clear any previous layout
            ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

            // split the dockspace into 2 nodes -- DockBuilderSplitNode takes in the following args in the following order
            //   window ID to split, direction, fraction (between 0 and 1), the final two setting let's us choose which id we want (which ever one we
            //   DON'T set as NULL, will be returned by the function)
            //                                                              out_id_at_dir is the id of the node in the direction we specified earlier,
            //                                                              out_id_at_opposite_dir is in the opposite direction
            auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.30f, nullptr, &dockspace_id);
            auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.30f, nullptr, &dockspace_id);
            auto dock_id_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.30f, nullptr, &dockspace_id);
            auto dock_id_center = ImGui::DockBuilderGetCentralNode(dockspace_id);
            auto dock_id_debug = ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Up, 0.30, nullptr, &dock_id_right);

            // we now dock our windows into the docking node we made above
            ImGui::DockBuilderDockWindow("Scene", dock_id_left);
            ImGui::DockBuilderDockWindow("Terminal", dock_id_down);
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
        gNodeSearchTexLen = strnlen_s(gNodeSearchText, 32);
    }

    renderNodeRecursively(scene);
    ImGui::End();

    // NOTE: terminal
    ImGui::Begin("Terminal");
    for(const auto& msg : gLogBuffer) {
        ImGui::TextAnsiUnformatted(msg.c_str(), nullptr);
    }
    ImGui::End();

    // NOTE: inspect
    ImGui::Begin("Inspect");
    //if (gSelectedNode != nullptr) renderNode(gSelectedNode);
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

    ImGui::Image((ImTextureID)Renderer::getRenderTargetTexture().id, wsize/*, ImVec2(0, 1), ImVec2(1, 0)*/);
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