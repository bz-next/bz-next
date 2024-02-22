#ifndef ONLINEMAPBROWSER_H
#define ONLINEMAPBROWSER_H

#include <vector>
#include "CacheManager.h"
#include "cURLManager.h"

class OnlineMapBrowser {
    public:
    OnlineMapBrowser();
    void draw(const char* title, bool* p_open);
};

#endif
