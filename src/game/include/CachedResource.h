#ifndef CACHEDRESOURCE_H
#define CACHEDRESOURCE_H

// Original code mostly only handled cached resources in the context of
// texture downloading. This is a generic cached resource to make referencing
// remotely-hosted content easier in general. No need to use the texture-specific
// Downloads() class, each resource can be handled independently, or grouped
// together using your own manager.

#include <vector>
#include <string>
#include "CacheManager.h"
#include "cURLManager.h"

class CachedResource : cURLManager
{
    public:
    CachedResource(const std::string &indexURL);
    void fetch();
    bool isComplete() const;
    bool isError() const;
    bool isInProgress() const;
    const std::vector<char>& getData() const { return _data; }
    virtual void finalization(char *data, unsigned int length, bool good);
    static void  setParams(bool check, long timeout);
    static int activeTransfer();
    private:
    virtual void collectData(char *ptr, int len);

    std::string url;

    std::vector<char> _data;
    bool _isComplete;
    bool _isError;
    bool _inProgress;
};

#endif
