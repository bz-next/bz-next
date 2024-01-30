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
#include "Magnum/Trade/AbstractImageConverter.h"
#include "Magnum/Trade/Trade.h"
#include "common.h"

// system headers
#include <vector>
#include <string>
#include <cctype>
#include <algorithm>

// common implementation headers

#include "TextUtils.h"
#include "global.h"
#include "MediaFile.h"
#include "ErrorHandler.h"
#include "OSFile.h"
#include "CacheManager.h"
#include "FileManager.h"

/*const int NO_VARIANT = (-1); */

using namespace Magnum;

static GL::Texture2D *magnumNoiseProc(MagnumProcTextureInit &init);

MagnumProcTextureInit magnumProcLoader[1];


MagnumTextureManager::MagnumTextureManager()
{
    // fill out the standard proc textures
    magnumProcLoader[0].name = "noise";
    magnumProcLoader[0].proc = magnumNoiseProc;

    lastImageID = -1;
    lastBoundID = -1;

    int i, numTextures;
    numTextures = bzcountof(magnumProcLoader);

    for (i = 0; i < numTextures; i++)
    {
        magnumProcLoader[i].manager = this;
        
        addTexture(
            magnumProcLoader[i].name.c_str(),
            magnumProcLoader[i].proc(magnumProcLoader[i]));
    }

    importer = manager.loadAndInstantiate("AnyImageImporter");
    converter = converterManager.loadAndInstantiate("AnyImageConverter");
}

MagnumTextureManager::~MagnumTextureManager()
{
    // we are done remove all textures
    for (TextureNameMap::iterator it = textureNames.begin(); it != textureNames.end(); ++it)
    {
        MagnumImageInfo &tex = it->second;
        if (tex.texture != NULL)
            delete tex.texture;
    }
    textureNames.clear();
}

std::vector<std::string> MagnumTextureManager::getTextureNames() {
    std::vector<std::string> keys;
    for (const auto& e: textureNames) {
        keys.push_back(e.first);
    }
    return keys;
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
        /*if (!importer || !importer->openFile(filename)) {
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
            .setSubImage(0, {}, *image);*/
        GL::Texture2D *texture = loadTexture(texInfo, reportFail);
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
    MagnumImageInfo& info = it->second;
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

    MagnumImageInfo& info = it->second;
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

void MagnumTextureManager::updateTextureFilters()
{
    // reset all texture filters to the current maxFilter
    TextureNameMap::iterator itr = textureNames.begin();
    while (itr != textureNames.end())
    {
        /*OpenGLTexture* texture = itr->second.texture;
        // getting, then setting re-clamps the filter level
        OpenGLTexture::Filter current = texture->getFilter();
        texture->setFilter(current);
        ++itr;*/
        // TODO: Implement this using the magnum texture class
    }

    // rebuild proc textures
    /*
    for (int i = 0; i < (int)bzcountof(procLoader); i++)
    {
        procLoader[i].manager = this;
        procLoader[i].proc(procLoader[i]);
    }*/
}

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
    MagnumImageInfo info;
    info.name = name;
    info.texture = texture;

    textureNames[name] = info;

    logDebugMessage(4,"Added texture %s\n", name);

    return info.texture;
}

GL::Texture2D* MagnumTextureManager::loadTexture(FileTextureInit &init, bool reportFail)
{
    
    std::string filename = init.name;
    if (filename == "") {
        return NULL;
    }
    if (CACHEMGR.isCacheFileType(init.name))
        filename = CACHEMGR.getLocalName(filename);

    // TODO: Support other file types JUST BY NOT REQUIRING THIS EXTENSION!!!
    // The current code is dumb in that it appends the extension last minute based on what exists...
    // Textures should be specified WITH extensions.
    bool addpng = true;
    if (filename.size() > 4) {
        std::string ext = filename.substr(filename.size()-4);
        std::cout << "ext " << ext << std::endl;
        // convert ext to lowercase
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){return std::tolower(c);});
        if (ext == ".png") {
            addpng = false;
            std::cout << "Detected png" << std::endl;
        }
    }
    if (addpng)
        filename += ".png";

    std::cout << "load Texture " << filename << std::endl;

    FileTextureInit texInfo;
    texInfo.name = filename;

    std::string fullfilepath = FileManager::instance().getFullFilePath(filename);

    std::cout << "the full path is " << fullfilepath << std::endl;

    if (!importer || !importer->openFile(fullfilepath)) {
        logDebugMessage(2,"Image not found or unloadable: %s\n", filename.c_str());
        return NULL;
    }
    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    if (!image)
    {
        logDebugMessage(2,"Image not found or unloadable: %s\n", filename.c_str());
        return NULL;
    }
    if (image->format() != PixelFormat::RGBA8Unorm && image->format() != PixelFormat::RGB8Unorm) {
        auto data = image->data();
        switch (image->format()) {
            case Magnum::PixelFormat::RG8Unorm:
            {
                Containers::Array<char> rgbaData{data.size()*2};
                int j = 0;
                for (int i = 0; i < data.size(); i += 2) {
                    rgbaData[j++] = data[i];
                    rgbaData[j++] = data[i];
                    rgbaData[j++] = data[i];
                    rgbaData[j++] = data[i+1];
                }
                image = Trade::ImageData2D{PixelFormat::RGBA8Unorm, image->size(), std::move(rgbaData)};
                break;
            }
            case Magnum::PixelFormat::R8Unorm:
            {
                Containers::Array<char> rgbaData{data.size()*4};
                int j = 0;
                for (int i = 0; i < data.size(); i += 1) {
                    rgbaData[j++] = data[i];
                    rgbaData[j++] = data[i];
                    rgbaData[j++] = data[i];
                    rgbaData[j++] = data[i];
                }
                image = Trade::ImageData2D{PixelFormat::RGBA8Unorm, image->size(), std::move(rgbaData)};
                break;
            }
            case Magnum::PixelFormat::RGB8Unorm:
            {
                Containers::Array<char> rgbaData{data.size()*4/3};
                int j = 0;
                for (int i = 0; i < data.size();) {
                    rgbaData[j++] = data[i++];
                    rgbaData[j++] = data[i++];
                    rgbaData[j++] = data[i++];
                    rgbaData[j++] = 32;
                }
                image = Trade::ImageData2D{PixelFormat::RGBA8Unorm, image->size(), std::move(rgbaData)};
                break;
            }
            default:
                Warning{} << "Unsupported pixel format " << image->format();
                break;
        }
    }

    GL::Texture2D *texture = new GL::Texture2D{};
    texture->setWrapping(GL::SamplerWrapping::Repeat)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear, GL::SamplerMipmap::Linear)
        .setMaxAnisotropy(GL::Sampler::maxMaxAnisotropy())
        .setStorage(1, GL::textureFormat(image->format()), image->size())
        .setSubImage(0, {}, *image)
        .generateMipmap();
    Warning{} << image->format();
    return texture;
}


/* --- Procs --- */

GL::Texture2D *magnumNoiseProc(MagnumProcTextureInit &init)
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
    Trade::ImageData2D image{PixelFormat::RGBA8Unorm, {(int)noiseSize, (int)noiseSize}, std::move(noise)};
    GL::Texture2D *texture = new GL::Texture2D{};
    texture->setWrapping(GL::SamplerWrapping::Repeat)
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
