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

#ifndef __MESH_UTILS_H__
#define __MESH_UTILS_H__


/* system interface headers */
#include <vector>

/* common interface headers */
#include "MeshObstacle.h"
#include "MagnumBZMaterial.h"


static inline void push2Ints(std::vector<int>& list, int i0, int i1)
{
    list.push_back(i0);
    list.push_back(i1);
    return;
}


static inline void push3Ints(std::vector<int>& list, int i0, int i1, int i2)
{
    list.push_back(i0);
    list.push_back(i1);
    list.push_back(i2);
    return;
}


static inline void push4Ints(std::vector<int>& list, int i0, int i1, int i2, int i3)
{
    list.push_back(i0);
    list.push_back(i1);
    list.push_back(i2);
    list.push_back(i3);
    return;
}


static inline void addFace(MeshObstacle* mesh,
                           std::vector<int>& verticesList,
                           std::vector<int>& normalsList,
                           std::vector<int>& texcoordsList,
                           const MagnumBZMaterial* material, int phydrv)
{
    // use the mesh defaults for smoothBounce, driveThrough, and shootThough
    //const MagnumBZMaterial* matref = MAGNUMMATERIALMGR.findMaterial(std::to_string(material->getLegacyIndex()));
    mesh->addFace(verticesList, normalsList, texcoordsList, material, phydrv,
                  false, false, false, false, false, false);
    verticesList.clear();
    normalsList.clear();
    texcoordsList.clear();
    return;
}


#endif  /* __MESH_UTILS_H__ */

// Local variables: ***
// mode: C++ ***
// tab-width: 4***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
