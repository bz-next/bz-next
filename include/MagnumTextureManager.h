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

#include <Magnum/GL/Texture.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Containers/Pointer.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "OpenGLTexture.h"
#include "Singleton.h"

struct FileTextureInit
{
    std::string       name;
    // Always use Linear Mipmap Linear for now
    //OpenGLTexture::Filter filter;
};


typedef  struct
{
    Magnum::GL::Texture2D *texture;
    std::string   name;
} ImageInfo;

class MagnumTextureManager;

struct ProcTextureInit
{
    std::string       name;
    MagnumTextureManager    *manager;
    Magnum::GL::Texture2D           *(*proc)(ProcTextureInit &init);
};


class MagnumTextureManager : public Singleton<MagnumTextureManager>
{
public:
    // No longer juggle texture IDs manually!
    //int getTextureID( const char* name, bool reportFail = true );
    Magnum::GL::Texture2D *getTexture(const char* name, bool reportFail = true);

    bool isLoaded(const std::string& name);
    bool removeTexture(const std::string& name);
    bool reloadTextures();
    bool reloadTextureImage(const std::string& name);

    void updateTextureFilters();
    void setTextureFilter(int texId, OpenGLTexture::Filter filter);
    OpenGLTexture::Filter getTextureFilter(int texId);

    // Now defunct, get the texture and associate it with the mesh instead
    bool bind ( int id );
    bool bind ( const char* name );

    const ImageInfo& getInfo ( int id );
    const ImageInfo& getInfo ( const char* name );

    OpenGLTexture::Filter getMaxFilter ( void );
    std::string getMaxFilterName ( void );
    void setMaxFilter ( OpenGLTexture::Filter filter );
    void setMaxFilter ( std::string filter );

    float GetAspectRatio ( int id );

protected:
    friend class Singleton<MagnumTextureManager>;

private:
    MagnumTextureManager();
    MagnumTextureManager(const MagnumTextureManager &tm);
    MagnumTextureManager& operator=(const MagnumTextureManager &tm);
    ~MagnumTextureManager();

    Magnum::GL::Texture2D *addTexture( const char*, Magnum::GL::Texture2D *texture );
    Magnum::GL::Texture2D* loadTexture( FileTextureInit &init, bool reportFail = true );

    typedef std::map<std::string, ImageInfo> TextureNameMap;
    typedef std::map<int, ImageInfo*> TextureIDMap;

    int       lastImageID;
    int       lastBoundID;
    TextureIDMap   textureIDs;
    TextureNameMap textureNames;

    Magnum::PluginManager::Manager<Magnum::Trade::AbstractImporter> manager;
    Corrade::Containers::Pointer<Magnum::Trade::AbstractImporter> importer;
};


#endif //MAGNUM_TEXTURE_MANAGER_H

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
