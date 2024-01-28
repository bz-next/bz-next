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

// interface headers
#include "MagnumTextureManager.h"
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Magnum/Magnum.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/ImageView.h>


// BZFlag common header
#include "Magnum/GL/GL.h"
#include "Magnum/GL/Sampler.h"
#include "common.h"

// system headers
#include <vector>
#include <string>

// common implementation headers

#include "bzfgl.h"
#include "TextUtils.h"
#include "global.h"
#include "MediaFile.h"
#include "ErrorHandler.h"
#include "OpenGLTexture.h"
#include "OSFile.h"

/*const int NO_VARIANT = (-1); */

using namespace Magnum;

static GL::Texture2D *noiseProc(ProcTextureInit &init);

ProcTextureInit procLoader[1];


MagnumTextureManager::MagnumTextureManager()
{
    // fill out the standard proc textures
    procLoader[0].name = "noise";
    procLoader[0].proc = noiseProc;

    lastImageID = -1;
    lastBoundID = -1;

    int i, numTextures;
    numTextures = bzcountof(procLoader);

    for (i = 0; i < numTextures; i++)
    {
        procLoader[i].manager = this;
        procLoader[i].proc(procLoader[i]);
    }

    importer = manager.loadAndInstantiate("AnyImageImporter");
}

MagnumTextureManager::~MagnumTextureManager()
{
    // we are done remove all textures
    for (TextureNameMap::iterator it = textureNames.begin(); it != textureNames.end(); ++it)
    {
        ImageInfo &tex = it->second;
        if (tex.texture != NULL)
            delete tex.texture;
    }
    textureNames.clear();
}

Magnum::GL::Texture2D *MagnumTextureManager::getTexture( const char* name, bool reportFail )
{
    if (!name)
    {
        logDebugMessage(2,"Could not get texture ID; no provided name\n");
        return NULL;
    }

    // see if we have the texture
    TextureNameMap::iterator it = textureNames.find(name);
    if (it != textureNames.end())
        return it->second.texture;
    else   // we don't have it so try and load it
    {

        OSFile osFilename(name); // convert to native format
        const std::string filename = osFilename.getOSName();

        FileTextureInit texInfo;
        texInfo.name = filename;
        //texInfo.filter = OpenGLTexture::LinearMipmapLinear;

        //OpenGLTexture *image = loadTexture(texInfo, reportFail);
        if (!importer || !importer->openFile(filename)) {
            logDebugMessage(2,"Image not found or unloadable: %s\n", name);
            return NULL;
        }
        Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
        if (!image)
        {
            logDebugMessage(2,"Image not found or unloadable: %s\n", name);
            return NULL;
        }
        GL::Texture2D *texture = new GL::Texture2D{};
        texture->setWrapping(GL::SamplerWrapping::ClampToEdge)
            .setMagnificationFilter(GL::SamplerFilter::Linear)
            .setMinificationFilter(GL::SamplerFilter::Linear)
            .setStorage(1, GL::textureFormat(image->format()), image->size())
            .setSubImage(0, {}, *image);
        return addTexture(name, texture);
    }
    return NULL;
}


bool MagnumTextureManager::isLoaded(const std::string& name)
{
    TextureNameMap::iterator it = textureNames.find(name);
    if (it == textureNames.end())
        return false;
    return true;
}


bool MagnumTextureManager::removeTexture(const std::string& name)
{
    TextureNameMap::iterator it = textureNames.find(name);
    if (it == textureNames.end())
        return false;

    // delete the OpenGLTexture
    ImageInfo& info = it->second;
    delete info.texture;
    info.texture = NULL;

    // clear the maps
    textureNames.erase(name);

    logDebugMessage(2,"MagnumTextureManager::removed: %s\n", name.c_str());

    return true;
}


bool MagnumTextureManager::reloadTextures()
{
    TextureNameMap::iterator it = textureNames.begin();
    while (it != textureNames.end())
    {
        reloadTextureImage(it->first);
        ++it;
    }
    return true;
}


bool MagnumTextureManager::reloadTextureImage(const std::string& name)
{
    TextureNameMap::iterator it = textureNames.find(name);
    if (it == textureNames.end())
        return false;

    ImageInfo& info = it->second;
    GL::Texture2D* oldTex = info.texture;
    // TODO: We are assuming linear mipmap linear for now
    //OpenGLTexture::Filter filter = oldTex->getFilter();

    // make the new texture object
    FileTextureInit fileInit;
    //fileInit.filter = OpenGLTexture::LinearMipmapLinear;
    fileInit.name = name;
    GL::Texture2D* newTex = loadTexture(fileInit, false);
    if (newTex == NULL)
    {
        // couldn't reload, leave it alone
        return false;
    }

    //  name and id fields are not changed
    info.texture = newTex;

    delete oldTex;

    return true;
}

/*
bool MagnumTextureManager::bind ( int id )
{
    TextureIDMap::iterator it = textureIDs.find(id);
    if (it == textureIDs.end())
    {
        logDebugMessage(1,"Unable to bind texture (by id): %d\n", id);
        return false;
    }

    if (id != lastBoundID)
    {
        it->second->texture->execute();
        lastBoundID = id;
    }
    return true;
}


bool MagnumTextureManager::bind ( const char* name )
{
    std::string nameStr = name;

    TextureNameMap::iterator it = textureNames.find(nameStr);
    if (it == textureNames.end())
    {
        logDebugMessage(1,"Unable to bind texture (by name): %s\n", name);
        return false;
    }

    int id = it->second.id;
    if (id != lastBoundID)
    {
        it->second.texture->execute();
        lastBoundID = id;
    }
    return true;
}


OpenGLTexture::Filter MagnumTextureManager::getMaxFilter ( void )
{
    return OpenGLTexture::getMaxFilter();
}


std::string MagnumTextureManager::getMaxFilterName ( void )
{
    OpenGLTexture::Filter maxFilter = OpenGLTexture::getMaxFilter();
    std::string name = OpenGLTexture::getFilterName(maxFilter);
    return name;
}


void MagnumTextureManager::setMaxFilter(std::string filter)
{
    const char** names = OpenGLTexture::getFilterNames();
    for (int i = 0; i < OpenGLTexture::getFilterCount(); i++)
    {
        if (filter == names[i])
        {
            setMaxFilter((OpenGLTexture::Filter) i);
            return;
        }
    }
    logDebugMessage(1,"setMaxFilter(): bad filter = %s\n", filter.c_str());
}


void MagnumTextureManager::setMaxFilter (OpenGLTexture::Filter filter )
{
    OpenGLTexture::setMaxFilter(filter);
    updateTextureFilters();
    return;
}


void MagnumTextureManager::updateTextureFilters()
{
    // reset all texture filters to the current maxFilter
    TextureNameMap::iterator itr = textureNames.begin();
    while (itr != textureNames.end())
    {
        OpenGLTexture* texture = itr->second.texture;
        // getting, then setting re-clamps the filter level
        OpenGLTexture::Filter current = texture->getFilter();
        texture->setFilter(current);
        ++itr;
    }

    // rebuild proc textures
    for (int i = 0; i < (int)bzcountof(procLoader); i++)
    {
        procLoader[i].manager = this;
        procLoader[i].proc(procLoader[i]);
    }
}


float MagnumTextureManager::GetAspectRatio ( int id )
{
    TextureIDMap::iterator it = textureIDs.find(id);
    if (it == textureIDs.end())
        return 0.0;

    return (float)it->second->y/(float)it->second->x;
}

const ImageInfo& MagnumTextureManager::getInfo ( int id )
{
    static ImageInfo   crapInfo;
    crapInfo.id = -1;
    TextureIDMap::iterator it = textureIDs.find(id);
    if (it == textureIDs.end())
        return crapInfo;

    return *(it->second);
}
const ImageInfo& MagnumTextureManager::getInfo ( const char* name )
{
    static ImageInfo crapInfo;
    crapInfo.id = -1;
    std::string nameStr = name;

    TextureNameMap::iterator it = textureNames.find(nameStr);
    if (it == textureNames.end())
        return crapInfo;

    return it->second;
}

*/

GL::Texture2D *MagnumTextureManager::addTexture( const char* name, GL::Texture2D *texture )
{
    if (!name || !texture)
        return NULL;

    // if the texture already exists kill it
    // this is why IDs are way better than objects for this stuff
    TextureNameMap::iterator it = textureNames.find(name);
    if (it != textureNames.end())
    {
        logDebugMessage(3,"Texture %s already exists, overwriting\n", name);
        delete it->second.texture;
    }
    ImageInfo info;
    info.name = name;
    info.texture = texture;

    textureNames[name] = info;

    logDebugMessage(4,"Added texture %s\n", name);

    return info.texture;
}

GL::Texture2D* MagnumTextureManager::loadTexture(FileTextureInit &init, bool reportFail)
{

    OSFile osFilename(init.name); // convert to native format
    const std::string filename = osFilename.getOSName();

    FileTextureInit texInfo;
    texInfo.name = filename;

    if (!importer || !importer->openFile(filename)) {
        logDebugMessage(2,"Image not found or unloadable: %s\n", filename.c_str());
        return NULL;
    }
    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    if (!image)
    {
        logDebugMessage(2,"Image not found or unloadable: %s\n", filename.c_str());
        return NULL;
    }
    GL::Texture2D *texture = new GL::Texture2D{};
    texture->setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::textureFormat(image->format()), image->size())
        .setSubImage(0, {}, *image);

    return texture;
}


void MagnumTextureManager::setTextureFilter(int texId, OpenGLTexture::Filter filter)
{
    // TODO: Support other filter modes
    // we only use linear mipmap linear for now...
    /*
    TextureIDMap::iterator it = textureIDs.find(texId);
    if (it == textureIDs.end())
    {
        logDebugMessage(1,"setTextureFilter() Couldn't find texid: %i\n", texId);
        return;
    }

    ImageInfo& image = *(it->second);
    OpenGLTexture* texture = image.texture;
    texture->setFilter(filter);*/
    logDebugMessage(2,"Unimplemented MagnumTextureManager::setTextureFilter");
    return;
}

/* --- Procs --- */

GL::Texture2D *noiseProc(ProcTextureInit &init)
{
    size_t noiseSize = 128;
    const size_t size = 4 * noiseSize * noiseSize;
    Containers::Array<char> noise{size};
    for (int i = 0; i < size; i += 4 )
    {
        char n = (unsigned char)floor(256.0 * bzfrand());
        noise[i+0] = n;
        noise[i+1] = n;
        noise[i+2] = n;
        noise[i+3] = n;
    }
    Trade::ImageData2D image{PixelFormat::RGB8Unorm, {(int)noiseSize, (int)noiseSize}, std::move(noise)};
    GL::Texture2D *texture = new GL::Texture2D{};
    texture->setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::textureFormat(image.format()), image.size())
        .setSubImage(0, {}, image);
    return texture;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
