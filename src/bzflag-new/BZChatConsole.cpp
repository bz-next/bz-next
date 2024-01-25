/* Based on code from the ImGui Interactive Manual, with the following license: */
/*
MIT License
Copyright (c) 2024 Pascal Thomet
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "ImGuiANSITextRenderer.h"
#include "BZChatConsole.h"

#include <cstdio>

BZChatConsole::BZChatConsole() {
    clearLog();
    memset(inputBuf, 0, sizeof(inputBuf));
    historyPos = -1;

    commands.push_back("/clear");
    autoScroll = true;
    scrollToBottom = false;
    addLog("Console Initialized");
}

BZChatConsole::~BZChatConsole() {
    clearLog();
    for (int i = 0; i < history.size(); ++i)
        free(history[i]);
}

void BZChatConsole::clearLog() {
    for (int i = 0; i < items.size(); ++i)
        free(items[i]);
    items.clear();
}

void BZChatConsole::addLog(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);
    items.push_back(Strdup(buf));
}
void BZChatConsole::draw(const char* title, bool* p_open) {
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title, p_open)) {
        ImGui::End();
        return;
    }
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, -1));
        for (const char* item : items) {
            ImGui::TextAnsiUnformatted(item, item + strlen(item));
        }
        if (scrollToBottom || (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            ImGui::SetScrollHereY(1.0f);
        scrollToBottom = false;
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::Separator();

    bool reclaim_focus = false;
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
    if (ImGui::InputText("##", inputBuf, IM_ARRAYSIZE(inputBuf), input_text_flags, &textEditCallbackStub, (void*)this)) {
        char *s = inputBuf;
        Strtrim(s);
        if (s[0])
            execCommand(s);
        strcpy(s, "");
        reclaim_focus = true;
    }
    ImGui::SetItemDefaultFocus();
    if (reclaim_focus)
        ImGui::SetKeyboardFocusHere(-1);
    ImGui::End();
}

void BZChatConsole::registerCommandCallback(CommandCallback&& fun) {
    commandCallbacks.emplace_back(std::move(fun));
}

void BZChatConsole::execCommand(const char* command_line) {
    addLog(command_line);
    bool handledLocally = false;
    historyPos = -1;
    for (int i = history.size() - 1; i >= 0; --i) {
        if (Stricmp(history[i], command_line) == 0) {
            free(history[i]);
            history.erase(history.begin() + i);
            break;
        }
    }
    history.push_back(strdup(command_line));
    if (Stricmp(command_line, "/CLEAR") == 0) {
        clearLog();
        handledLocally = true;
    }
    scrollToBottom = true;
    if (handledLocally) return;
    for (CommandCallback cb: commandCallbacks) {
        cb(command_line);
    }
}
int BZChatConsole::textEditCallbackStub(ImGuiInputTextCallbackData* data) {
    BZChatConsole* console = (BZChatConsole*)data->UserData;
    return console->textEditCallback(data);
}
int BZChatConsole::textEditCallback(ImGuiInputTextCallbackData* data) {
    switch (data->EventFlag) {
        case ImGuiInputTextFlags_CallbackCompletion:
        {   // tab-completion for server commands could go here...
            break;
        }
        case ImGuiInputTextFlags_CallbackHistory:
        {
            const int prev_history_pos = historyPos;
            if (data->EventKey == ImGuiKey_UpArrow) {
                if (historyPos == -1) {
                    historyPos = history.size() - 1;
                } else if (historyPos > 0) {
                    historyPos--;
                }
            } else if (data->EventKey == ImGuiKey_DownArrow) {
                if (historyPos != -1)
                    if (++historyPos >= history.size())
                        historyPos = -1;
            }

            if (prev_history_pos != historyPos) {
                const char* history_str = (historyPos > 0) ? history[historyPos] : "";
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, history_str);
            }
            break;
        }
    }
    return 0;
}