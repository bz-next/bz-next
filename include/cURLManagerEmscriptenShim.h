#ifndef CURLMANAGEREMSCRIPTENSHIM_H
#define CURLMANAGEREMSCRIPTENSHIM_H

#ifndef TARGET_EMSCRIPTEN
#error "Do not include this file!"
#endif

#include <emscripten/fetch.h>

// bzflag common header
#include "common.h"

#include "network.h"

// system headers
#include <string>
#include <map>
#include <vector>

using EMProgressCallbackT = void(*)(emscripten_fetch_t *);

class cURLManager
{
public:
    cURLManager();
    virtual ~cURLManager();

    enum timeCondition
    {
        None,
        ModifiedSince
    };

    void addHandle();
    void removeHandle();

    void setTimeout(long timeout);
    void setNoBody();
    void setGetMode();
    void setHTTPPostMode();
    void setPostMode(std::string postData);
    void setRequestFileTime(bool request);
    void setURL(const std::string &url);
    void setURLwithNonce(const std::string &url);
    void setProgressFunction(EMProgressCallbackT func, const void* data);
    void setTimeCondition(timeCondition condition, time_t &t);
    void setInterface(const std::string &interfaceIP);
    void setUserAgent(const std::string &userAgent);
    void setDNSCachingTime(long time);

    void addFormData(const char *key, const char *value);

    bool getFileTime(time_t &t);
    bool getFileSize(double &size);

    virtual void collectData(char *ptr, int len);
    virtual void finalization(char *data, unsigned int length, bool good);

    static int    fdset(fd_set &read, fd_set &write);
    static bool   perform();
    void      performWait();

protected:
    void         *theData;
    unsigned int  theLen;
private:

    void      infoComplete(int result);

    emscripten_fetch_attr_t attr;
    bool      added;
    std::string   fileUrl;
    std::string   interfaceIP;
    std::string   userAgent;
    std::string   postData;

    struct curl_httppost* formPost;
    struct curl_httppost* formLast;

    static void   setup();

    static void downloadSucceeded(emscripten_fetch_t *fetch);
};


typedef enum
{
    eImage,
    eSound,
    eFont,
    eFile,
    eUnknown
} teResourceType;

typedef struct
{
    teResourceType    resType;
    std::string       URL;
    std::string       filePath;
    std::string       fileName;
} trResourceItem;

class ResourceGetter : cURLManager
{
public:
    ResourceGetter();
    virtual ~ResourceGetter();

    void addResource(trResourceItem &item);
    void flush(void);

    virtual void finalization(char *data, unsigned int length, bool good);

protected:
    bool itemExists(trResourceItem &item);
    void getResource(void);

    std::vector<trResourceItem> resources;
    bool doingStuff;
};

#endif