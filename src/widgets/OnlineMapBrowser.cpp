#include "OnlineMapBrowser.h"
#include "CacheManager.h"
#include "include/OnlineMapBrowser.h"

CachedMapIndex::CachedMapIndex(const std::string &indexURL) : cURLManager()
{
    CacheManager::CacheRecord oldrec;
    url = indexURL;

    bool cached = CACHEMGR.findURL(indexURL, oldrec);
    if (cached) {
        // Use the cached file, oldrec.name
    } else {
        setTimeout(5.0);
        setRequestFileTime(true);
        std::string msg = "Downloading " + indexURL;
        setTimeCondition(ModifiedSince, oldrec.date);
        addHandle();
    }
}

//void CachedMapIndex::finalization(char *data, unsigned int length, bool good);