#ifndef BZWTEXTEDITOR_H
#define BZWTEXTEDITOR_H

#include <string>

#include "TextEditor.h"

class BZWTextEditor {
    public:

    using SaveCallback = void(*)(const std::string& filename, const std::string& data);
    using ReloadCallback = void(*)(const std::string& filename, const std::string& data);

    BZWTextEditor();
    void draw(const char* title, bool* p_open);
    void loadFile(const std::string& filename, const std::string& data, bool loadFile = false);
    void setSaveCallback(SaveCallback cb);
    void setReloadCallback(ReloadCallback cb);
    private:
    std::string currentFile;
    TextEditor editor;
    SaveCallback onSave;
    ReloadCallback onReload;
};

#endif