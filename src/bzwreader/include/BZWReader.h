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

#ifndef __BZWREADER_H__
#define __BZWREADER_H__

// bzflag common header
#include "common.h"

#include "network.h"

// system headers
#include <iostream>
#include <string>
#include <vector>

/* bzflag common headers */
#include "BZWError.h"

#ifndef TARGET_EMSCRIPTEN
#include "cURLManager.h"
#endif

class WorldFileObject;
class WorldInfo;

class BZWReader
#ifndef TARGET_EMSCRIPTEN
: cURLManager
#endif
{
public:
    BZWReader(std::string filename);
    BZWReader(std::string filename, const std::string& filedata);
    ~BZWReader();

    // external interface
    WorldInfo *defineWorldFromFile();

private:
    // functions for internal use
    void readToken(char *buffer, int n);
    bool readWorldStream(std::vector<WorldFileObject*>& wlist,
                         class GroupDefinition* groupDef);
    void finalization(char *data, unsigned int length, bool good);

    // stream to open
    std::string location;
    std::istream *input;

    // data/dependent objects
    BZWError *errorHandler;

    // no default constructor
    BZWReader();

    std::string httpData;
};

#endif

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
