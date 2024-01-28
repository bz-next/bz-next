#ifndef BZTEXTUREBROWSER_H
#define BZTEXTUREBROWSER_H

#include <Magnum/ImGuiIntegration/Context.hpp>

#include "MagnumTextureManager.h"

class BZTextureBrowser {
    public:
    BZTextureBrowser();
    void draw(const char* title, bool* p_open);
    private:
    int itemCurrent;
};

#endif
