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

#pragma once

// 1st
#include "common.h"

// System headers
#include <vector>

// common interface headers
#include "MeshDrawInfo.h"

class MeshDrawMgr
{
public:
    MeshDrawMgr(const MeshDrawInfo* drawInfo);
    ~MeshDrawMgr();

    void executeSet(int lod, int set, bool useNormals, bool useTexcoords);
    void executeSetGeometry(int lod, int set);

private:
    void rawExecuteCommands(int lod, int set);

    void makeLists();
    void freeLists();
    static void initContext(void* data);
    static void freeContext(void* data);

private:
    const MeshDrawInfo* drawInfo;

    using LodList = std::vector<int>;
    std::vector<LodList> lodLists;
};


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
