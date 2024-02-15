/* bzflag
 * Copyright (c) 1993-2021 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

// class interface header

#include "cURLManager.h"

// system includes
#include <string.h>

// common implementation headers
#include "bzfio.h"
#include "Protocol.h"
#include "TextUtils.h"

void onFailurePlsWork(emscripten_fetch_t *fetch) {
    std::cout << "Failure!" << fetch->url << " " << fetch->status << std::endl;
    emscripten_fetch_close(fetch);
}

cURLManager::cURLManager()
{
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_PERSIST_FILE;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = onFailurePlsWork;
    attr.userData = (void*)this;

    theData   = NULL;
    theLen    = 0;
}

cURLManager::~cURLManager()
{
    free(theData);
}

void cURLManager::setup()
{
}

void cURLManager::downloadSucceeded(emscripten_fetch_t *fetch)
{
    std::cout << "downloadSucceeded" << std::endl;
    ((cURLManager*)fetch->userData)->collectData((char*)fetch->data, fetch->numBytes);
    std::cout << "freeing fetch thing" << std::endl;
    emscripten_fetch_close(fetch);
}

void cURLManager::setTimeout(long timeout)
{
    attr.timeoutMSecs = 1000L*timeout;
}

void cURLManager::setNoBody()
{
    // Unused
}

void cURLManager::setGetMode()
{
    // Unused
}

void cURLManager::setPostMode(std::string _postData)
{
    // Only used by ServerList
}

void cURLManager::setHTTPPostMode()
{
    // Unused
}

void cURLManager::setURL(const std::string &url)
{
    fileUrl = url;
}

void cURLManager::setURLwithNonce(const std::string &url)
{
    // only the default list server is known to support the nonce parameter
    const std::string nonce = (strcasecmp(url.c_str(), DefaultListServerURL) == 0) ? TextUtils::format("?nocache=%lu",
                              time(0)) : "";
    setURL(url + nonce);
}

void cURLManager::setProgressFunction(EMProgressCallbackT func, const void* data)
{
    attr.onprogress = func;
}

void cURLManager::setRequestFileTime(bool request)
{
}

void cURLManager::addHandle()
{
    emscripten_fetch(&attr, fileUrl.c_str());
}

void cURLManager::removeHandle()
{
}

void cURLManager::finalization(char *, unsigned int, bool)
{
}

void cURLManager::collectData(char* ptr, int len)
{
    std::cout << "cURLManager::collectData" << std::endl;
    unsigned char *newData = (unsigned char *)realloc(theData, theLen + len);
    if (!newData)
        logDebugMessage(1,"memory exhausted\n");
    else
    {
        std::cout << "memcpy " << theLen << std::endl;
        memcpy(newData + theLen, ptr, len);
        std::cout << "done" << std::endl;
        theLen += len;
        theData = newData;
    }
    // Just call infoComplete directly...
    std::cout << "calling infoComplete" << std::endl;
    infoComplete(0);
}

void cURLManager::performWait()
{
    // Primarily used by bzfs and bzwreader on non-emscripten
}

int cURLManager::fdset(fd_set &read, fd_set &write)
{
    // Only used by bzfs
    return 0;
}

bool cURLManager::perform()
{
    return false;
}

void cURLManager::infoComplete(int result)
{
    std::cout << "Calling finalization" << std::endl;
    finalization((char *)theData, theLen, result == 0);
    free(theData);
    theData = NULL;
    theLen  = 0;
}

bool cURLManager::getFileTime(time_t &t)
{
    t = (time_t)0;
    return true;
}

bool cURLManager::getFileSize(double &size)
{
    // Only used by NewVersionMenu
    return false;
}

void cURLManager::setTimeCondition(timeCondition condition, time_t &t)
{
}

void cURLManager::setInterface(const std::string &_interfaceIP)
{
    // Only used by listserverconnection
}

void cURLManager::setUserAgent(const std::string &_userAgent)
{
    // Only used by bzfs
}

void cURLManager::addFormData(const char *key, const char *value)
{
    // Unused
}

void cURLManager::setDNSCachingTime(long time)
{
    // Unused
}

//**************************ResourceGetter*************************

ResourceGetter::ResourceGetter() : cURLManager()
{
    doingStuff = false;
}

ResourceGetter::~ResourceGetter()
{

}

void ResourceGetter::addResource ( trResourceItem &item )
{
    resources.push_back(item);

    if (!doingStuff)
        getResource();
}

void ResourceGetter::flush ( void )
{
    resources.clear();
    doingStuff = false;
}

void ResourceGetter::finalization(char *data, unsigned int length, bool good)
{
    if (!resources.size() || !doingStuff)
        return; // we are suposed to be done

    // this is who we are suposed to be geting
    trResourceItem item = resources[0];
    resources.erase(resources.begin());
    if (good)
    {
        // save the thing
        FILE *fp = fopen(item.filePath.c_str(),"wb");
        if (fp)
        {
            if (fwrite(data, length, 1, fp) != 1)
                logDebugMessage(1, "Unable to write to file with CURL\n");
            fclose(fp);
        }

        // maybe send a message here saying we did it?
    }

    // do the next one if we must
    getResource();
}

bool ResourceGetter::itemExists ( trResourceItem &item )
{
    // save the thing
    FILE *fp = fopen(item.filePath.c_str(),"rb");
    if (fp)
    {
        fclose(fp);
        return true;
    }
    return false;
}

void ResourceGetter::getResource ( void )
{
    while ( resources.size() && itemExists(resources[0]) )
        resources.erase(resources.begin());

    if ( !resources.size() )
        doingStuff = false;
    else
    {
        trResourceItem item = resources[0];

        doingStuff = true;
        setURL(item.URL);
        addHandle();
    }
}



// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
