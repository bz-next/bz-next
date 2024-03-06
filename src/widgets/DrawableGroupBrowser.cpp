#include "DrawableGroupBrowser.h"

#include <sstream>

#include <imgui.h>
#include <Magnum/ImGuiIntegration/Widgets.h>
#include <string>

#include "DrawableGroupManager.h"

using namespace Magnum;

DrawableGroupBrowser::DrawableGroupBrowser() {
}

void DrawableGroupBrowser::draw(const char *title, bool *p_open) {
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin(title, p_open);
    ImGui::BeginChild("DrawGroupList");
    auto groups = DGRPMGR.getGroups();
    for (auto e: groups) {
            std::string entry = e.first + ": " + std::to_string(e.second->size()) + " elements";
            ImGui::Text(entry.c_str());
    }
    ImGui::EndChild();
    ImGui::End();
}
