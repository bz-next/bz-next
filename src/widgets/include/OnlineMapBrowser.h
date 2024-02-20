#ifndef ONLINEMAPBROWSER_H
#define ONLINEMAPBROWSER_H

#include "CacheManager.h"
#include "cURLManager.h"

class CachedMapIndex : cURLManager
{
    public:
    CachedMapIndex(const std::string &indexURL);
    virtual void finalization(char *data, unsigned int length, bool good);
    static void  setParams(bool check, long timeout);
    static int activeTransfer();
    private:
    virtual void collectData(char *ptr, int len);

    std::string url;
    static bool          checkForCache;
    static long          httpTimeout;
    static int        textureCounter;
    static int                byteTransferred;
    bool            timeRequest;
};

class OnlineMapBrowser {
    public:
    OnlineMapBrowser();
    void draw(const char* title, bool* p_open);
};

#endif
