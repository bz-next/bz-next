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
#ifndef MAGNUM_TEXTURE_MANAGER_H
#define MAGNUM_TEXTURE_MANAGER_H

#include <string>
#include <map>
#include <vector>

#include <Magnum/GL/Texture.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Containers/Pointer.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/AbstractImageConverter.h>

#include "Magnum/GL/GL.h"
#include "Singleton.h"

struct FileTextureInit
{
    std::string       name;
    // Always use Linear Mipmap Linear for now
    //OpenGLTexture::Filter filter;
};

struct TextureData
{
    Magnum::GL::Texture2D *texture;
    unsigned int width, height;
    bool hasAlpha;
};

typedef  struct
{
    std::string   name;
    TextureData data;
} MagnumImageInfo;

class MagnumTextureManager;

struct MagnumProcTextureInit
{
    std::string       name;
    MagnumTextureManager    *manager;
    TextureData           (*proc)(MagnumProcTextureInit &init);
};


class MagnumTextureManager : public Singleton<MagnumTextureManager>
{
public:
    // No longer juggle texture IDs manually!
    //int getTextureID( const char* name, bool reportFail = true );
    TextureData getTexture(const char* name, bool reportFail = true);

    bool isLoaded(const std::string& name);
    bool removeTexture(const std::string& name);
    bool reloadTextures();
    bool reloadTextureImage(const std::string& name);

    void clear();

    std::vector<std::string> getTextureNames();

    void updateTextureFilters();

protected:
    friend class Singleton<MagnumTextureManager>;

private:
    MagnumTextureManager();
    MagnumTextureManager(const MagnumTextureManager &tm);
    MagnumTextureManager& operator=(const MagnumTextureManager &tm);
    ~MagnumTextureManager();

    TextureData addTexture( std::string name, TextureData data );
    TextureData loadTexture( FileTextureInit &init, bool reportFail = true );

    typedef std::map<std::string, MagnumImageInfo> TextureNameMap;
    typedef std::map<int, MagnumImageInfo*> TextureIDMap;

    int       lastImageID;
    int       lastBoundID;
    TextureIDMap   textureIDs;
    TextureNameMap textureNames;

    Magnum::PluginManager::Manager<Magnum::Trade::AbstractImporter> manager;
    Corrade::Containers::Pointer<Magnum::Trade::AbstractImporter> importer;
    Magnum::PluginManager::Manager<Magnum::Trade::AbstractImageConverter> converterManager;
    Corrade::Containers::Pointer<Magnum::Trade::AbstractImageConverter> converter;
};


#endif //MAGNUM_TEXTURE_MANAGER_H

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
