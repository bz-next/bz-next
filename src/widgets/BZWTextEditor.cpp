#include <fstream>

#include "BZWTextEditor.h"
#include <imgui.h>

#include <Magnum/ImGuiIntegration/Widgets.h>
#include <sstream>

using namespace Magnum;

BZWTextEditor::BZWTextEditor() {
    onSave = NULL;
    onReload = NULL;
    currentFile = "untitled.bzw";
    editor.SetHandleKeyboardInputs(true);
    editor.SetLanguageDefinition(TextEditor::LanguageDefinition::BZW());
}

void BZWTextEditor::loadFile(const std::string& filename, const std::string& data, bool loadFile) {
    currentFile = filename;

	if (loadFile) {
		std::ifstream fs(filename);
		std::ostringstream ss;
		ss << fs.rdbuf();
		editor.SetText(ss.str());
	} else {
		editor.SetText(data);
	}

    
}

void BZWTextEditor::setSaveCallback(SaveCallback cb) {
    onSave = cb;
}

void BZWTextEditor::setReloadCallback(ReloadCallback cb) {
    onReload = cb;
}

void BZWTextEditor::draw(const char *title, bool *p_open) {
auto cpos = editor.GetCursorPosition();
        ImGui::Begin(title, p_open, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
		ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
		if (ImGui::BeginMenuBar())
		{
            if (ImGui::MenuItem("Save"))
            {
                if (onSave) {
                    onSave(currentFile, editor.GetText());
                }
            }
			if (ImGui::BeginMenu("Edit"))
			{
                editor.SetReadOnly(false);
				bool ro = editor.IsReadOnly();

				if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
					editor.Undo();
				if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
					editor.Redo();

				ImGui::Separator();

				if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
					editor.Copy();
				if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
					editor.Cut();
				if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
					editor.Delete();
				if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
					editor.Paste();

				ImGui::Separator();

				if (ImGui::MenuItem("Select all", nullptr, nullptr))
					editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Themes"))
			{
				if (ImGui::MenuItem("Dark palette"))
					editor.SetPalette(TextEditor::GetDarkPalette());
				if (ImGui::MenuItem("Light palette"))
					editor.SetPalette(TextEditor::GetLightPalette());
				if (ImGui::MenuItem("Retro blue palette"))
					editor.SetPalette(TextEditor::GetRetroBluePalette());
				ImGui::EndMenu();
			}
            if (ImGui::MenuItem("Update 3D View")) {
                if (onReload) {
                    onReload(currentFile, editor.GetText());
                }
            }
			ImGui::EndMenuBar();
		}

		ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
			editor.IsOverwrite() ? "Ovr" : "Ins",
			editor.CanUndo() ? "*" : " ",
			editor.GetLanguageDefinition().mName.c_str(), currentFile.c_str());

		editor.Render("TextEditor");
		ImGui::End();
    
}



		