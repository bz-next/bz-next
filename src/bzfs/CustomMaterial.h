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

#ifndef __CUSTOM_MATERIAL_H__
#define __CUSTOM_MATERIAL_H__

/* interface header */
#include "WorldFileObject.h"

/* common headers */
#include "MagnumBZMaterial.h"

class CustomMaterial : public WorldFileObject
{
public:
    CustomMaterial();
    ~CustomMaterial();
    virtual bool read(const char *cmd, std::istream& input);
    virtual void writeToManager() const;
    bool usesManager()
    {
        return true;
    }

private:
    mutable MagnumBZMaterial material;
};

#endif  /* __CUSTOM_MATERIAL_H__ */

// Local variables: ***
// mode: C++ ***
// tab-width: 4***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
