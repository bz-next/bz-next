#include "SceneObjectBrowser.h"

#include <sstream>

#include <imgui.h>
#include <Magnum/ImGuiIntegration/Widgets.h>
#include <string>

#include "SceneObjectManager.h"

using namespace Magnum;

SceneObjectBrowser::SceneObjectBrowser() {
}

void SceneObjectBrowser::draw(const char *title, bool *p_open) {
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin(title, p_open);
    ImGui::BeginChild("SceneObjList");
    auto groups = SOMGR.getObjs();
    for (auto e: groups) {
            std::string entry = e.first;
            ImGui::Text(entry.c_str());
    }
    ImGui::EndChild();
    ImGui::End();
}
