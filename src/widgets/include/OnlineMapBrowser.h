#ifndef ONLINEMAPBROWSER_H
#define ONLINEMAPBROWSER_H

#include <vector>
#include <string>
#include <map>

#include "json.hpp"

#include "CacheManager.h"
#include "cURLManager.h"
#include "CachedResource.h"

struct MapInfo {
    std::string name;
    std::string file;
    std::string license;
};
//void from_json(const nlohmann::json& j, MapInfo& mi);

struct MapList {
    std::vector<MapInfo> maps;
};

struct MapIndex {
    std::map<std::string, MapList> index;
};
//void from_json(const nlohmann::json& j, AuthorInfo& ai);
class OnlineMapBrowser {
    public:
    using OnMapDownloadComplete = void(*)(const CachedResource&);
    OnlineMapBrowser();
    void draw(const char* title, bool* p_open);
    static void setOnDownloadCompleteCallback(OnMapDownloadComplete cb) { _cb = cb; }
    private:
    void maybeLoadFetchedData();

    CachedResource mapIndex;
    static CachedResource *mapRsc;
    std::string fetchedData;

    nlohmann::json mapIndexJSON;

    MapIndex _mi;

    static OnMapDownloadComplete _cb;

};

#endif
