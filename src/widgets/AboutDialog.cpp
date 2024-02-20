#include "AboutDialog.h"
#include <imgui.h>

#include <Magnum/GL/Texture.h>
#include <Magnum/ImGuiIntegration/Widgets.h>

using namespace Magnum;

AboutDialog::AboutDialog() {
}

void AboutDialog::draw(const char *title, bool *p_open) {

    ImGui::Begin(title, p_open, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("BZ-Next Rendering Engine");
    ImGui::Text("Released under MPL and LGPL License, (c) 2024 bz-next");
    ImGui::Separator();
    ImGui::Text("Uses code from the following projects:");
    ImGui::Text("BZFlag");
    ImGui::Text("(c) Tim Riker, under LGPL and MPL License");
    ImGui::Text("Magnum, Corrade, Magnum-Integration, Magnum-Plugins, Toolchains");
    ImGui::Text("(c) Vladimir Vondrus under MIT license");
    ImGui::Text("Dear ImGUI");
    ImGui::Text("(c) Omar Cornut, under MIT License");
    ImGui::Text("ImGUI ANSI Text Renderer");
    ImGui::Text("(c) David Dovodov, under MIT License");
    ImGui::Text("ImGuiColorTextEdit");
    ImGui::Text("(c) BalazsJako, under MIT License");
    ImGui::Text("For license details, please see");
    ImGui::Text("https://bz-next.github.io/bz-next/bz-next");
    ImGui::End();
}
