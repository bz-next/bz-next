#include "BZTextureBrowser.h"
#include "MagnumTextureManager.h"
#include <imgui.h>

#include <Magnum/GL/Texture.h>
#include <Magnum/ImGuiIntegration/Widgets.h>

using namespace Magnum;

BZTextureBrowser::BZTextureBrowser() {
    itemCurrent = 0;
}

void BZTextureBrowser::draw(const char *title, bool *p_open) {
    MagnumTextureManager &tm = MagnumTextureManager::instance();
    std::vector<std::string> names = tm.getTextureNames();
    std::string names_cc;
    for (const auto& e: names) {
        names_cc+= e + std::string("\0", 1);
    }
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin(title, p_open, ImGuiWindowFlags_NoScrollbar);
    if (names_cc.size() >= 0) {
        ImGui::Combo("Texture Name", &itemCurrent, names_cc.c_str(), names_cc.size());
    } else {
        ImGui::Text("No Textures Loaded");
    }
    if (names_cc.size() >= 0) {
        TextureData tex = tm.getTexture(names[itemCurrent].c_str());
        ImVec2 ws = ImGui::GetContentRegionAvail();
        float width = fmin((float)ws.x, (float)tex.width);
        float height = fmin((float)tex.height, width/(float)tex.width*(float)tex.height);
        ImGuiIntegration::image(*tex.texture, {width, height});
    }
    ImGui::End();
}
