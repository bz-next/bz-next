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
    ImGui::Begin(title, p_open, ImGuiWindowFlags_AlwaysAutoResize);
    if (names_cc.size() >= 0) {
        ImGui::Combo("Texture Name", &itemCurrent, names_cc.c_str(), names_cc.size());
    } else {
        ImGui::Text("No Textures Loaded");
    }
    if (names_cc.size() >= 0) {
        GL::Texture2D *tex = tm.getTexture(names[itemCurrent].c_str());
        auto size = tex->imageSize(0);
        ImGuiIntegration::image(*tex, {(float)size.x(), (float)size.y()});
    }
    ImGui::End();
}
