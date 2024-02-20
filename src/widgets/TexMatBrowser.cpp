#include "TexMatBrowser.h"

#include <sstream>

#include <imgui.h>
#include <Magnum/ImGuiIntegration/Widgets.h>

#include "TextureMatrix.h"

using namespace Magnum;

TexMatBrowser::TexMatBrowser() {
}

void TexMatBrowser::draw(const char *title, bool *p_open) {
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin(title, p_open);
    ImGui::BeginChild("TexMatList");
    auto list = TEXMATRIXMGR.getMatList();
    for (auto e: list) {
        if (e == NULL) continue;
        if (ImGui::TreeNode(e->getName().c_str())) {
            std::ostringstream ostr;
            e->print(ostr, "");
            ImGui::TextWrapped(ostr.str().c_str());
            ImGui::TreePop();
        }
    }
    ImGui::EndChild();
    ImGui::End();
}
