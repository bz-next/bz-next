#include "ObstacleBrowser.h"
#include "MagnumTextureManager.h"
#include "MagnumBZMaterial.h"
#include "ObstacleMgr.h"
#include "Obstacle.h"
#include <imgui.h>

#include <Magnum/GL/Texture.h>
#include <Magnum/ImGuiIntegration/Widgets.h>

using namespace Magnum;

ObstacleBrowser::ObstacleBrowser() {
    itemCurrent = 0;
}

void displayObstacle(const Obstacle &o) {
    ImGui::Text("Type: %s", o.getType());
    ImGui::Text("isValid: %d", o.isValid());
    ImGui::Text("isFlatTop: %d", o.isFlatTop());
    ImGui::Text("packSize: %d", o.packSize());
    const float *p = o.getPosition();
    ImGui::Text("position: %f %f %f", p[0], p[1], p[2]);
    p = o.getSize();
    ImGui::Text("size: %f %f %f", p[0], p[1], p[2]);
    ImGui::Text("rotation: %d", o.getRotation());
    ImGui::Text("width: %d", o.getWidth());
    ImGui::Text("breadth: %d", o.getBreadth());
    ImGui::Text("height: %d", o.getHeight());
}

void ObstacleBrowser::draw(const char *title, bool *p_open) {

    std::vector<std::string> names = MAGNUMMATERIALMGR.getMaterialNames();
    std::string names_cc;
    for (const auto& e: names) {
        names_cc += e + std::string("\0", 1);
    }
    ImGui::Begin(title, p_open, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::BeginChild("ObjLists", ImVec2(400, 260));
    if (ImGui::TreeNode("Walls")) {
        const auto& list = OBSTACLEMGR.getWalls();
        for (int i = 0; i < list.size(); ++i) {
            if (ImGui::TreeNode((void*)(intptr_t)i, "Wall %d", i)) {
                displayObstacle(*list[i]);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Boxes")) {
        const auto& list = OBSTACLEMGR.getBoxes();
        for (int i = 0; i < list.size(); ++i) {
            if (ImGui::TreeNode((void*)(intptr_t)i, "Boxe %d", i)) {
                displayObstacle(*list[i]);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Pyramids")) {
        const auto& list = OBSTACLEMGR.getPyrs();
        for (int i = 0; i < list.size(); ++i) {
            if (ImGui::TreeNode((void*)(intptr_t)i, "Pyramid %d", i)) {
                displayObstacle(*list[i]);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Bases")) {
        const auto& list = OBSTACLEMGR.getBases();
        for (int i = 0; i < list.size(); ++i) {
            if (ImGui::TreeNode((void*)(intptr_t)i, "Base %d", i)) {
                displayObstacle(*list[i]);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Teleporters")) {
        const auto& list = OBSTACLEMGR.getTeles();
        for (int i = 0; i < list.size(); ++i) {
            if (ImGui::TreeNode((void*)(intptr_t)i, "Teleporter %d", i)) {
                displayObstacle(*list[i]);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Meshes")) {
        const auto& list = OBSTACLEMGR.getMeshes();
        for (int i = 0; i < list.size(); ++i) {
            if (ImGui::TreeNode((void*)(intptr_t)i, "Mesh %d", i)) {
                displayObstacle(*list[i]);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Arcs")) {
        const auto& list = OBSTACLEMGR.getArcs();
        for (int i = 0; i < list.size(); ++i) {
            if (ImGui::TreeNode((void*)(intptr_t)i, "Arc %d", i)) {
                displayObstacle(*list[i]);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Cones")) {
        const auto& list = OBSTACLEMGR.getCones();
        for (int i = 0; i < list.size(); ++i) {
            if (ImGui::TreeNode((void*)(intptr_t)i, "Cone %d", i)) {
                displayObstacle(*list[i]);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Spheres")) {
        const auto& list = OBSTACLEMGR.getSpheres();
        for (int i = 0; i < list.size(); ++i) {
            if (ImGui::TreeNode((void*)(intptr_t)i, "Sphere %d", i)) {
                displayObstacle(*list[i]);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Tetras")) {
        const auto& list = OBSTACLEMGR.getTetras();
        for (int i = 0; i < list.size(); ++i) {
            if (ImGui::TreeNode((void*)(intptr_t)i, "Tetra %d", i)) {
                displayObstacle(*list[i]);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::EndChild();

    ImGui::End();
}
