#ifndef BZMATERIALBROWSER_H
#define BZMATERIALBROWSER_H

#include <Magnum/ImGuiIntegration/Context.hpp>

#include "MagnumTextureManager.h"

class BZMaterialBrowser {
    public:
    BZMaterialBrowser();
    void draw(const char* title, bool* p_open);
    private:
    int itemCurrent;
};

#endif
