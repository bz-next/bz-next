#include "BZMaterialBrowser.h"
#include "MagnumTextureManager.h"
#include "MagnumBZMaterial.h"
#include <imgui.h>

#include <Magnum/GL/Texture.h>
#include <Magnum/ImGuiIntegration/Widgets.h>

using namespace Magnum;

BZMaterialBrowser::BZMaterialBrowser() {
    itemCurrent = 0;
}

void BZMaterialBrowser::draw(const char *title, bool *p_open) {

    std::vector<std::string> names = MAGNUMMATERIALMGR.getMaterialNames();
    std::string names_cc;
    for (const auto& e: names) {
        names_cc += e + std::string("\0", 1);
    }
    ImGui::Begin(title, p_open, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Combo("Material Name", &itemCurrent, names_cc.c_str(), names_cc.size());
    /*
    if (names_cc.size() >= 0) {
        GL::Texture2D *tex = tm.getTexture(names[itemCurrent].c_str());
        auto size = tex->imageSize(0);
        ImGuiIntegration::image(*tex, {(float)size.x(), (float)size.y()});
    }*/
    ImGui::Separator();
    if (names.size() >= 0 && itemCurrent < names.size()) {
        const MagnumBZMaterial *mat = MAGNUMMATERIALMGR.findMaterial(names[itemCurrent]);
        std::vector<std::string> aliases = mat->getAliases();
        std::string aliases_cc;
        for (const auto& e: aliases) {
            aliases_cc += e + std::string("\0", 1);
        }
        int cur = 0;
        ImGui::Combo("Aliases", &cur, aliases_cc.c_str(), aliases_cc.size());
        ImGui::Text("Dynamic Color: %d", mat->getDynamicColor());
        const float *color = mat->getAmbient();
        ImGui::Text("Ambient: %f %f %f", color[0], color[1], color[2]);
        color = mat->getDiffuse();
        ImGui::Text("Diffuse: %f %f %f", color[0], color[1], color[2]);
        color = mat->getSpecular();
        ImGui::Text("Specular: %f %f %f", color[0], color[1], color[2]);
        color = mat->getEmission();
        ImGui::Text("Emission: %f %f %f", color[0], color[1], color[2]);
        ImGui::Text("Shininess: %f", mat->getShininess());
        ImGui::Text("Occluder: %d", mat->getOccluder());
        ImGui::Text("Group Alpha: %d", mat->getGroupAlpha());
        ImGui::Text("No Radar: %d", mat->getNoRadar());
        ImGui::Text("No Shadow: %d", mat->getNoShadow());
        ImGui::Text("No Culling: %d", mat->getNoCulling());
        ImGui::Text("No Sorting: %d", mat->getNoSorting());
        ImGui::Text("No Lighting: %d", mat->getNoLighting());
        ImGui::Text("Alpha Threshold: %d", mat->getAlphaThreshold());
        ImGui::Text("Texture Count: %d", mat->getTextureCount());
        for (int i = 0; i < mat->getTextureCount(); ++i) {
            ImGui::Text("  Texture %d: %s", i, mat->getTexture(i).c_str());
            ImGui::Text("    Local Name: %s", i, mat->getTextureLocal(i).c_str());
            ImGui::Text("    Matrix: %d", mat->getTextureMatrix(i));
            ImGui::Text("    Combine Mode: %d", mat->getCombineMode(i));
            ImGui::Text("    Use Tex Alpha: %d", mat->getUseTextureAlpha(i));
            ImGui::Text("    Use Color: %d", mat->getUseColorOnTexture(i));
            ImGui::Text("    Use Sphere Map: %d", mat->getUseSphereMap(i));
        }
        ImGui::Text("Invisible: %d", mat->isInvisible());
    }

    ImGui::End();
}
