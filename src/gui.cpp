#include "gui.hpp"
#include "frame.hpp"
#include "app.hpp"
#include "scene.hpp"
#include "camera.h"
#include "logger.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <string>

void renderNodeRecursively(Frame* frame) {
	if (frame != nullptr && ImGui::TreeNode(frame->getName().c_str())) {
		for (const auto& child : frame->getChilds()) {
            renderNodeRecursively(child.get());
		}

		ImGui::TreePop();
		ImGui::Separator();
	}
}

ImVec2 lastWsize;

void Gui::render() {
    auto* scene = App::get()->getScene();
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
            auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, nullptr, &dockspace_id);
            auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.2f, nullptr, &dockspace_id);
            auto dock_id_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);
            auto dock_id_center = ImGui::DockBuilderGetCentralNode(dockspace_id);
            // we now dock our windows into the docking node we made above
            ImGui::DockBuilderDockWindow("Scene", dock_id_left);
            ImGui::DockBuilderDockWindow("Console", dock_id_down);
            ImGui::DockBuilderDockWindow("Inspect", dock_id_right);
            ImGui::DockBuilderDockWindow("View", dock_id_center->ID);
            ImGui::DockBuilderFinish(dockspace_id);
        }
    }

    ImGui::End();

    // NOTE: scene section
    ImGui::Begin("Scene");
    renderNodeRecursively(scene);
    ImGui::End();

    // NOTE: console
    ImGui::Begin("Console");
    for(auto& line : Logger::getGuiOutputBuffer()) {
        ImGui::Text("%s", line.c_str());
    }
    ImGui::End();

    // NOTE: inspect
    ImGui::Begin("Inspect");
    //if (gSelectedNode != nullptr) renderNode(gSelectedNode);
    ImGui::End();

    // NOTE: game view
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

    ImGui::Image((ImTextureID)Renderer::getRenderTargetTexture().id, wsize, ImVec2(0, 1), ImVec2(1, 0));
    ImGui::EndChild();
    ImGui::End();

    // NOTE: debug, camera settings
    ImGui::Begin("Debug");
    //auto cam = scene->getCamera();
    //ImGui::InputFloat3("Camera pos", (float*)&cam->Position);
    //ImGui::Separator();

    //// NOTE: scene section
    //if (ImGui::Button("Clear Scene")) {
    //    gSelectedNode = nullptr;
    //    scene->clear();
    //}

    //static char fileBuff[250] = "assets/MISSIONS/MISE17-VEZENI";
    //ImGui::InputText("###model", fileBuff, 250);
    //ImGui::SameLine();
    //if (ImGui::Button("Load scene")) {
    //    if (strlen(fileBuff)) {
    //        scene->loadScene(fileBuff);
    //    }
    //    // ImGui::OpenPopup("Texture atlas");
    //}

    //renderTextureAtlas();
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}