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

#include <functional>
#include <vector>
#include <cstring>
#include <cctype>
#include <Magnum/ImGuiIntegration/Context.hpp>

class BZChatConsole {
    using CommandCallback = std::function<void(const char*)>;
    public:
        BZChatConsole();
        ~BZChatConsole();

        void clearLog();
        void addLog(const char* fmt, ...) IM_FMTARGS(2);
        void draw(const char* title, bool* p_open);
        void execCommand(const char* command_line);
        void registerCommandCallback(CommandCallback&& fun);
        static int textEditCallbackStub(ImGuiInputTextCallbackData* data);
        int textEditCallback(ImGuiInputTextCallbackData* data);

    private:
        char inputBuf[256];
        ImVector<char*> items;
        ImVector<const char*> commands;
        ImVector<char*> history;
        int historyPos;
        ImGuiTextFilter filter;
        bool autoScroll;
        bool scrollToBottom;

        std::vector<CommandCallback> commandCallbacks;

        static int   Stricmp(const char* s1, const char* s2)         { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
        static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
        static char* Strdup(const char* s)                           { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
        static void  Strtrim(char* s)                                { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }
};

