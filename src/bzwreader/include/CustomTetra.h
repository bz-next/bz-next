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

#ifndef __CUSTOMTETRA_H__
#define __CUSTOMTETRA_H__

/* interface header */
#include "WorldFileObstacle.h"

/* local interface header */
#include "WorldInfo.h"

/* system header */
#include <string>

/* common interface header */
#include "MagnumBZMaterial.h"

class CustomTetra : public WorldFileObstacle
{
public:
    CustomTetra();
    virtual bool read(const char *cmd, std::istream& input);
    virtual void writeToGroupDef(GroupDefinition*) const;

private:
    int vertexCount;

    float vertices[4][3];
    float normals[4][3][3];
    float texcoords[4][3][2];
    bool useNormals[4];
    bool useTexcoords[4];
    MagnumBZMaterial materials[4];
};

#endif  /* __CUSTOMTETRA_H__ */

// Local variables: ***
// mode: C++ ***
// tab-width: 4***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
