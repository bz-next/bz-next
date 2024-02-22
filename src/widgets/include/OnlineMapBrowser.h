#ifndef ONLINEMAPBROWSER_H
#define ONLINEMAPBROWSER_H

#include <vector>
#include <string>
#include "CacheManager.h"
#include "cURLManager.h"
#include "CachedResource.h"

class OnlineMapBrowser {
    public:
    OnlineMapBrowser();
    void draw(const char* title, bool* p_open);
    private:
    void maybeLoadFetchedData();
    CachedResource mapIndex;
    std::string fetchedData;
};

#endif
