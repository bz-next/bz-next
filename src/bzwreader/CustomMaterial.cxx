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

#include "common.h"

/* interface header */
#include "CustomMaterial.h"

/* system implementation headers */
#include <sstream>

/* bzfs implementation headers */
#include "ParseMaterial.h"

/* common implementation headers */
#include "MagnumBZMaterial.h"


CustomMaterial::CustomMaterial()
{
    return;
}


CustomMaterial::~CustomMaterial()
{
    return;
}


bool CustomMaterial::read(const char *cmd, std::istream& input)
{
    bool materror;

    if (parseMaterials(cmd, input, &material, 1, materror))
    {
        if (materror)
            return false;
    }
    else
        return WorldFileObject::read(cmd, input);

    return true;
}


void CustomMaterial::writeToManager() const
{
    material.setName(name);

    if ((name.size() > 0) && (MAGNUMMATERIALMGR.findMaterial(name) != NULL))
    {
        std::cout << "warning: duplicate material name"
                  << " (" << name << ")" << std::endl;
        std::cout << "	 the first material will be used" << std::endl;
    }

    const MagnumBZMaterial* matref = MAGNUMMATERIALMGR.addLegacyIndexedMaterial(&material);

    int index = MAGNUMMATERIALMGR.getIndex(matref);
    if (index < 0)
        std::cout << "CustomMaterial::write: material didn't register" << std::endl;
    return;
}


// Local variables: ***
// mode: C++ ***
// tab-width: 4***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
