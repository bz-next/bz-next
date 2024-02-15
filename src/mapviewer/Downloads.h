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

#ifndef DOWNLOADS_H
#define DOWNLOADS_H

#include "common.h"

#include "cURLManager.h"

/* system interface headers */
#include <string>


namespace Downloads
{
void startDownloads(bool doDownloads,
                    bool updateDownloads,
                    bool referencing);
void finalizeDownloads();
void removeTextures(); // free the downloaded GL textures
bool requestFinalized();
}

bool authorizedServer(const std::string& hostname);
bool parseHostname(const std::string& url, std::string& hostname);

class CachedTexture : cURLManager
{
public:
    CachedTexture(const std::string &texUrl);

    virtual void finalization(char *data, unsigned int length, bool good);

    static void  setParams(bool check, long timeout);
    static int   activeTransfer();
private:

    virtual void collectData(char* ptr, int len);

    std::string          url;
    static bool          checkForCache;
    static long          httpTimeout;
    static int        textureCounter;
    static int                byteTransferred;
    bool            timeRequest;
};

#endif

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
