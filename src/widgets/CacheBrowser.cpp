#include "CacheBrowser.h"

#include "CacheManager.h"

#include <imgui.h>
#include <Magnum/ImGuiIntegration/Widgets.h>

#include <ctime>
#include <locale>

using namespace Magnum;

CacheBrowser::CacheBrowser() {
}

void CacheBrowser::draw(const char *title, bool *p_open) {

    auto getTimeStr = [](const time_t val) {
        char timeStr[std::size("yyyy-mm-ddThh:mm:ssZ")];
        std::strftime(std::data(timeStr), std::size(timeStr),
            "%FT%TZ", std::gmtime(&val));
        return std::string(timeStr);
    };
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin(title, p_open);
    ImGui::BeginChild("CacheList");
    auto list = CACHEMGR.getCacheList();
    for (auto e: list) {
        if (ImGui::TreeNode(e.name.c_str())) {
            ImGui::Text("Name: %s", e.name.c_str());
            ImGui::Text("Size: %d", e.size);
            ImGui::Text("Date: %s", getTimeStr(e.date).c_str());
            ImGui::Text("Used Date: %s", getTimeStr(e.usedDate).c_str());
            ImGui::Text("URL: %s", e.url.c_str());
            ImGui::Text("Key: %s", e.key.c_str());
            ImGui::TreePop();
        }
    }
    ImGui::EndChild();
    ImGui::End();
}
