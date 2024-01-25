/*
MIT License

Copyright (c) 2024 David Dovodov

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

#include <vector>
#include <string>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

namespace ImGui
{
    bool ParseColor(const char* s, ImU32* col, int* skipChars);

    void ImFont_RenderAnsiText(const ImFont* font,
        ImDrawList* draw_list,
        float size,
        ImVec2 pos,
        ImU32 col,
        const ImVec4& clip_rect,
        const char* text_begin,
        const char* text_end,
        float wrap_width = 0.0f,
        bool cpu_fine_clip = false);

    void ImDrawList_AddAnsiText(ImDrawList* drawList,
        const ImFont* font,
        float font_size,
        const ImVec2& pos,
        ImU32 col,
        const char* text_begin,
        const char* text_end = NULL,
        float wrap_width = 0.0f,
        const ImVec4* cpu_fine_clip_rect = NULL);

    void RenderAnsiText(ImVec2 pos, const char* text, const char* text_end, bool hide_text_after_hash);

    void RenderAnsiTextWrapped(ImVec2 pos, const char* text, const char* text_end, float wrap_width);

    void TextAnsiUnformatted(const char* text, const char* text_end);

    void TextAnsiV(const char* fmt, va_list args);

    void TextAnsiColoredV(const ImVec4& col, const char* fmt, va_list args);

    void TextAnsiColored(const ImVec4& col, const char* fmt, ...);
}
