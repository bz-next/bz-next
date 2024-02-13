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
#include <Corrade/Utility/Resource.h>
#include <Magnum/Magnum.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/ImageView.h>


// BZFlag common header
#include "Magnum/GL/GL.h"
#include "Magnum/GL/Sampler.h"
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

static TextureData magnumNoiseProc(MagnumProcTextureInit &init);

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

        auto result = magnumProcLoader[i].proc(magnumProcLoader[i]);
        
        addTexture(
            magnumProcLoader[i].name.c_str(),
            {result.texture, result.width, result.height, result.hasAlpha});
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
        if (tex.data.texture != NULL)
            delete tex.data.texture;
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

TextureData MagnumTextureManager::getTexture( const char* name, bool reportFail )
{
    if (!name)
    {
        logDebugMessage(2,"Could not get texture ID; no provided name\n");
        return {NULL, 0, 0};
    }

    // see if we have the texture
    TextureNameMap::iterator it = textureNames.find(name);
    if (it != textureNames.end())
        return it->second.data;
    else   // we don't have it so try and load it
    {

        OSFile osFilename(name); // convert to native format
        const std::string filename = osFilename.getOSName();

        FileTextureInit texInfo;
        texInfo.name = filename;

        TextureData data = loadTexture(texInfo, reportFail);
        return addTexture(name, data);
    }
    return {NULL, 0, 0, false};
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
    delete info.data.texture;
    info.data.texture = NULL;

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

void MagnumTextureManager::clear()
{
    for (TextureNameMap::iterator it = textureNames.begin(); it != textureNames.end(); ++it)
    {
        MagnumImageInfo &tex = it->second;
        if (tex.data.texture != NULL)
            delete tex.data.texture;
    }
    textureNames.clear();
}


bool MagnumTextureManager::reloadTextureImage(const std::string& name)
{
    TextureNameMap::iterator it = textureNames.find(name);
    if (it == textureNames.end())
        return false;

    MagnumImageInfo& info = it->second;
    GL::Texture2D* oldTex = info.data.texture;

    // make the new texture object
    FileTextureInit fileInit;

    fileInit.name = name;
    TextureData newTex = loadTexture(fileInit, false);
    if (newTex.texture == NULL)
    {
        // couldn't reload, leave it alone
        return false;
    }

    //  name and id fields are not changed
    info.data = newTex;

    delete oldTex;

    return true;
}

void MagnumTextureManager::updateTextureFilters()
{
    // We just assume linear mipmap linear for now
}

TextureData MagnumTextureManager::addTexture( std::string name, TextureData data )
{
    if (!data.texture)
        return {NULL, 0, 0};

    // if the texture already exists kill it
    // this is why IDs are way better than objects for this stuff
    TextureNameMap::iterator it = textureNames.find(name);
    if (it != textureNames.end())
    {
        logDebugMessage(3,"Texture %s already exists, overwriting\n", name.c_str());
        delete it->second.data.texture;
    }
    MagnumImageInfo info;
    info.name = name;
    info.data = data;

    textureNames[name] = info;

    logDebugMessage(4,"Added texture %s\n", name.c_str());

    return info.data;
}

TextureData MagnumTextureManager::loadTexture(FileTextureInit &init, bool reportFail)
{
    
    std::string filename = init.name;
    if (filename == "") {
        return {NULL, 0, 0};
    }
    if (CACHEMGR.isCacheFileType(init.name))
        filename = CACHEMGR.getLocalName(filename);

    // TODO: Support other file types JUST BY NOT REQUIRING THIS EXTENSION!!!
    // The current code is dumb in that it appends the extension last minute based on what exists...
    // Textures should be specified WITH extensions.
    bool addpng = true;
    if (filename.size() > 4) {
        std::string ext = filename.substr(filename.size()-4);
        // convert ext to lowercase
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){return std::tolower(c);});
        if (ext == ".png") {
            addpng = false;
        }
    }
    if (addpng)
        filename += ".png";

    //std::cout << "load Texture " << filename << std::endl;

    FileTextureInit texInfo;
    texInfo.name = filename;

    // Check if we have this in our packed resources
    Utility::Resource rs{"bzflag-texture-data"};
    if (rs.hasFile(filename)) {
        Warning{} << "We have" << filename.c_str() << "in our packed resources.";
    }

    std::string fullfilepath = FileManager::instance().getFullFilePath(filename);

    // Try to load from packed resources if we can
    if (rs.hasFile(filename)) {
        if (!importer || !importer->openData(rs.getRaw(filename))) {
            logDebugMessage(2,"Image not found or unloadable: %s\n", filename.c_str());
            return {NULL, 0, 0, false};
        }
    } else {
        if (!importer || !importer->openFile(fullfilepath)) {
            logDebugMessage(2,"Image not found or unloadable: %s\n", filename.c_str());
            return {NULL, 0, 0, false};
        }
    }
    
    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    if (!image)
    {
        logDebugMessage(2,"Image not found or unloadable: %s\n", filename.c_str());
        return {NULL, 0, 0, false};
    }
    bool hasAlpha = true;
    if (image->format() == PixelFormat::RGB8Unorm)
        hasAlpha = false;
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
                    rgbaData[j++] = 0xFF;
                }
                image = Trade::ImageData2D{PixelFormat::RGBA8Unorm, image->size(), std::move(rgbaData)};
                break;
            }
            default:
                Warning{} << "Unsupported pixel format " << image->format();
                break;
        }
    }

    // Check if any alpha values are actually < 1.0
    // This helps us disable blending for non-transparent textures
    if (hasAlpha) {
        hasAlpha = false;
        auto data = image->data();
        for (int i = 0; i < data.size(); i+= 4) {
            if ((unsigned char)data[i+3] != 0xFF) {
                hasAlpha = true;
                break;
            }
        }
    }

    GL::Texture2D *texture = new GL::Texture2D{};
    texture->setWrapping(GL::SamplerWrapping::Repeat)
#if defined(MAGNUM_TARGET_GLES) || defined(MAGNUM_TARGET_GLES2)
        // If targeting GLES2 assume less capable system
        .setMagnificationFilter(GL::SamplerFilter::Nearest)
        .setMinificationFilter(GL::SamplerFilter::Nearest, GL::SamplerMipmap::Nearest)
        .setMaxAnisotropy(1.0f)
#else
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear, GL::SamplerMipmap::Linear)
        .setMaxAnisotropy(GL::Sampler::maxMaxAnisotropy())
#endif
        .setStorage(4, GL::textureFormat(image->format()), image->size())
        .setSubImage(0, {}, *image)
        .generateMipmap();
    return {
        texture,
        (unsigned int)image->size()[0],
        (unsigned int)image->size()[1],
        hasAlpha};
}


/* --- Procs --- */

TextureData magnumNoiseProc(MagnumProcTextureInit &init)
{
    unsigned int noiseSize = 128;
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
    return {texture, noiseSize, noiseSize, false};
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
