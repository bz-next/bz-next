#include "CachedResource.h"

#include "cURLManager.h"
#include "CacheManager.h"
#include "FileManager.h"
#include "include/CachedResource.h"

#include <ctime>
#include <string>
#include <istream>
#include <algorithm>
#include <iterator>

CachedResource::CachedResource(const std::string &indexURL) : cURLManager()
{
    url = indexURL;
    setURL(url);

    _isComplete = false;
    _isError = false;
}

void CachedResource::fetch()
{
    if (_isComplete) return;
    CacheManager::CacheRecord oldrec;
    CACHEMGR.findURL(url, oldrec);
    setTimeout(5.0); // TODO: Grab this from BZDB
    setRequestFileTime(true);
    std::string msg = "Downloading " + url;
    setTimeCondition(ModifiedSince, oldrec.date);
    addHandle();
}

bool CachedResource::isComplete() const
{
    return _isComplete;
}

bool CachedResource::isError() const
{
    return _isError;
}

void CachedResource::finalization(char *data, unsigned int length, bool good)
{
    if (good) {
        if (length) {
            // Cache the data for next time
            time_t filetime;
            getFileTime(filetime);
            CacheManager::CacheRecord rec;
            rec.url = url;
            rec.size = length;
            rec.date = filetime;
            CACHEMGR.addFile(rec, data);
            // Load the data into our object, since we'll need it.
            _data.reserve(length);
            for (unsigned int i = 0; i < length; ++i) {
                _data.push_back(data[i]);
            }
            _isComplete = true;
        }
    } else {
        // Maybe we didn't download it and it's in the cache
        // This might happen if the remote resource isn't newer than what we already have
        // In that case, try to load it!
        CacheManager::CacheRecord rec;
        if (CACHEMGR.findURL(url, rec)) {
            std::string localname = CACHEMGR.getLocalName(rec.name);
            auto is = FILEMGR.createDataInStream(localname, true);
            std::copy(std::istream_iterator<char>(*is), std::istream_iterator<char>(), std::back_inserter(_data));
        } else {
            // We weren't able to find it... all hope is lost
            _isError = true;
        }
        // Regardless of outcome, we are done.
        _isComplete = true;
    }
}

void CachedResource::collectData(char *ptr, int len)
{
    cURLManager::collectData(ptr, len);
}