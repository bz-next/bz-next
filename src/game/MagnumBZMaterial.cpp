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

#include <string.h>

#include <string>
#include <sstream>
#include <string>

#include "MagnumTextureManager.h"
#include "MagnumBZMaterial.h"

#include "TextureMatrix.h"
#include "DynamicColor.h"
#include "Pack.h"
#include "Team.h"

/****************************************************************************/
//
// MagnumBZMaterialManager
//

MagnumBZMaterialManager MAGNUMMATERIALMGR;

void MagnumBZMaterialManager::loadDefaultMaterials() {
    MagnumBZMaterial defaultMat;
    defaultMat.setName("_DefaultMaterial");
    MAGNUMMATERIALMGR.addMaterial(&defaultMat);

    auto &tm = MagnumTextureManager::instance();
    tm.getTexture("boxwall");
    tm.getTexture("roof");

    auto *boxwallmat = new MagnumBZMaterial();
    boxwallmat->setName("boxWallMaterial");
    boxwallmat->addTexture("boxwall");
    boxwallmat->setUseTextureAlpha(false);
    MAGNUMMATERIALMGR.addMaterial(boxwallmat);

    auto *roofmat = new MagnumBZMaterial();
    roofmat->setName("boxTopMaterial");
    roofmat->addTexture("roof");
    roofmat->setUseTextureAlpha(false);
    MAGNUMMATERIALMGR.addMaterial(roofmat);

    auto *pyrmat = new MagnumBZMaterial();
    pyrmat->setName("pyrWallMaterial");
    pyrmat->addTexture("pyrwall");
    pyrmat->setUseTextureAlpha(false);
    MAGNUMMATERIALMGR.addMaterial(pyrmat);

    TeamColor teams[4] = {
        RedTeam,
        GreenTeam,
        BlueTeam,
        PurpleTeam
    };
    for (auto t: teams) {
        auto *basewall = new MagnumBZMaterial();
        basewall->setName(Team::getImagePrefix(t) + "baseWallMaterial");
        basewall->addTexture(Team::getImagePrefix(t) + "basewall");
        basewall->setUseTextureAlpha(false);
        MAGNUMMATERIALMGR.addMaterial(basewall);

        auto *basetop = new MagnumBZMaterial();
        basetop->setName(Team::getImagePrefix(t) + "baseTopMaterial");
        basetop->addTexture(Team::getImagePrefix(t) + "basetop");
        basetop->setUseTextureAlpha(false);
        MAGNUMMATERIALMGR.addMaterial(basetop); 
    }

    auto *wallmat = new MagnumBZMaterial();
    wallmat->setName("wallMaterial");
    wallmat->addTexture("wall");
    MAGNUMMATERIALMGR.addMaterial(wallmat);

    auto *cautionMat = new MagnumBZMaterial();
    cautionMat->setName("cautionMaterial");
    cautionMat->addTexture("caution");
    MAGNUMMATERIALMGR.addMaterial(cautionMat);

    /*auto *teleLink = new MagnumBZMaterial();
    teleLink->setName("TeleLinkMaterial");
    teleLink->addTexture("telelink");
    teleLink->setAlphaThreshold(0.5);
    MAGNUMMATERIALMGR.addMaterial(teleLink);*/

    auto *ground = new MagnumBZMaterial();
    ground->setName("GroundMaterial");
    ground->addTexture("std_ground");
    ground->setUseTextureAlpha(false);
    MAGNUMMATERIALMGR.addMaterial(ground);
}

MagnumBZMaterialManager::MagnumBZMaterialManager()
{
    clear(false);
    return;
}


MagnumBZMaterialManager::~MagnumBZMaterialManager()
{
    clear(false);
    return;
}

std::vector<std::string> MagnumBZMaterialManager::getMaterialNames()
{
    std::vector<std::string> names;
    for (const auto& e: materials) {
        names.push_back(e->getName());
    }
    return names;
}


void MagnumBZMaterialManager::clear(bool loadDefaults)
{
    legacyMaterialIndex = 0;
    for (unsigned int i = 0; i < materials.size(); i++)
        delete materials[i];
    materials.clear();
    unnamedCount = unnamedAliasCount = duplicateNameCount = 0;
    if (loadDefaults)
        loadDefaultMaterials();
    return;
}

void MagnumBZMaterialManager::forceLoadTextures()
{
    for (auto m: materials) {
        if (m->getTextureCount() != 0) {
            for (int i = 0; i < m->getTextureCount(); ++i)
                MagnumTextureManager::instance().getTexture(m->getTexture(i).c_str());
        }
    }
}

const MagnumBZMaterial* MagnumBZMaterialManager::addLegacyIndexedMaterial(const MagnumBZMaterial* material)
{
    std::ostringstream namebuilder;
    namebuilder << "_LegacyMaterialIndex" << legacyMaterialIndex;

    MagnumBZMaterial *indexedMat = new MagnumBZMaterial(*material);
    indexedMat->addAlias(namebuilder.str());
    indexedMat->setLegacyIndex(legacyMaterialIndex);

    const auto* mat = addMaterial(indexedMat);

    delete indexedMat;

    legacyMaterialIndex += 1;

    return mat;
}

const MagnumBZMaterial* MagnumBZMaterialManager::addMaterial(const MagnumBZMaterial* material)
{
    for (unsigned int i = 0; i < materials.size(); i++)
    {
        if (*material == *(materials[i]))
        {
            const std::string& name = material->getName();
            if (name.size() > 0)
                materials[i]->addAlias(name);
            else
                materials[i]->addAlias("_UnnamedAlias"+std::to_string(unnamedAliasCount++));
            return materials[i];
        }
    }
    MagnumBZMaterial* newMat = new MagnumBZMaterial(*material);
    if (newMat->getName() == "") {
        newMat->setName("_Unnamed"+std::to_string(unnamedCount++));
    }
    if (findMaterial(newMat->getName()) != NULL)
        newMat->setName("_NameCollisionWith" + newMat->getName() + std::to_string(duplicateNameCount++));
    materials.push_back(newMat);
    return newMat;
}


const MagnumBZMaterial* MagnumBZMaterialManager::findMaterial(const std::string& target) const
{
    if (target.size() <= 0)
        return NULL;
    // Lookup legacy indexed materials by a string containing the index number
    else if ((target[0] >= '0') && (target[0] <= '9'))
    {
        int index = atoi (target.c_str());
        if ((index < 0))
            return NULL;
        for (auto m: materials) {
            if (m->getLegacyIndex() == index) {
                return m;
            }
        }
        return NULL;
    }
    else
    {
        for (unsigned int i = 0; i < materials.size(); i++)
        {
            const MagnumBZMaterial* mat = materials[i];
            // check the base name
            if (target == mat->getName())
                return mat;
            // check the aliases
            const std::vector<std::string>& aliases = mat->getAliases();
            for (unsigned int j = 0; j < aliases.size(); j++)
            {
                if (target == aliases[j])
                    return mat;
            }
        }
        return NULL;
    }
}


const MagnumBZMaterial* MagnumBZMaterialManager::getMaterial(int id) const
{
    const MagnumBZMaterial *ret;
    ret = findMaterial(std::to_string(id));
    if (ret) {
        return ret;
    }
    ret = findMaterial("_DefaultMaterial");
    return ret;
}

/* This function is for interfacing with legacy code that refers to materials by index.
   This is bad practice because it implies that the material manager is unchanging, and
   can lead to confusion and bugs. Instead, we map indices to material names and search
   based on that. This function makes adapting old code easier. */
int MagnumBZMaterialManager::getIndex(const MagnumBZMaterial* material) const
{
    for (unsigned int i = 0; i < materials.size(); i++)
    {
        if (material == materials[i]) {
            if (materials[i]->getLegacyIndex() != -1) {
                return materials[i]->getLegacyIndex();
            }
            return -1;
        }
    }
    return -1;
}


void* MagnumBZMaterialManager::pack(void* buf)
{
    buf = nboPackUInt(buf, (unsigned int)materials.size());
    for (unsigned int i = 0; i < materials.size(); i++)
        buf = materials[i]->pack(buf);

    return buf;
}


const void* MagnumBZMaterialManager::unpack(const void* buf)
{
    unsigned int i;
    uint32_t count;
    buf = nboUnpackUInt (buf, count);
    std::cout << "Unpacking " << count << std::endl;
    for (i = 0; i < count; i++)
    {
        MagnumBZMaterial* mat = new MagnumBZMaterial;
        buf = mat->unpack(buf);
        std::cout << "Unpacked " << mat->getName() << std::endl;
        addLegacyIndexedMaterial(mat);
        delete mat;
    }
    return buf;
}


int MagnumBZMaterialManager::packSize()
{
    int fullSize = sizeof (uint32_t);
    for (unsigned int i = 0; i < materials.size(); i++)
        fullSize += materials[i]->packSize();
    return fullSize;
}


void MagnumBZMaterialManager::print(std::ostream& out, const std::string& indent) const
{
    for (unsigned int i = 0; i < materials.size(); i++)
        materials[i]->print(out, indent);
    return;
}


void MagnumBZMaterialManager::printMTL(std::ostream& out, const std::string& indent) const
{
    for (unsigned int i = 0; i < materials.size(); i++)
        materials[i]->printMTL(out, indent);
    return;
}


void MagnumBZMaterialManager::printReference(std::ostream& out,
                                       const MagnumBZMaterial* mat) const
{
    if (mat == NULL)
    {
        out << "-1";
        return;
    }
    int index = getIndex(mat);
    if (index == -1)
    {
        out << "-1";
        return;
    }
    if (mat->getName().size() > 0)
    {
        out << mat->getName();
        return;
    }
    else
    {
        out << index;
        return;
    }
}


void MagnumBZMaterialManager::makeTextureList(TextureSet& set, bool referenced) const
{
    set.clear();
    for (unsigned int i = 0; i < materials.size(); i++)
    {
        const MagnumBZMaterial* mat = materials[i];
        for (int j = 0; j < mat->getTextureCount(); j++)
        {
            if (mat->getReference() || !referenced)
                set.insert(mat->getTexture(j));
        }
    }
    return;
}


void MagnumBZMaterialManager::setTextureLocal(const std::string& url,
                                        const std::string& local)
{
    for (unsigned int i = 0; i < materials.size(); i++)
    {
        MagnumBZMaterial* mat = materials[i];
        for (int j = 0; j < mat->getTextureCount(); j++)
        {
            if (mat->getTexture(j) == url)
                mat->setTextureLocal(j, local);
        }
    }
    return;
}


/****************************************************************************/
//
// MagnumBZMaterial
//

MagnumBZMaterial MagnumBZMaterial::defaultMaterial;
std::string MagnumBZMaterial::nullString = "";


void MagnumBZMaterial::reset()
{
    dynamicColor = -1;
    const float defAmbient[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    const float defDiffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    const float defSpecular[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const float defEmission[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    memcpy (ambient, defAmbient, sizeof(ambient));
    memcpy (diffuse, defDiffuse, sizeof(diffuse));
    memcpy (specular, defSpecular, sizeof(specular));
    memcpy (emission, defEmission, sizeof(emission));
    shininess = 0.0f;
    alphaThreshold = 0.0f;
    occluder = false;
    groupAlpha = false;
    noRadar = false;
    noShadow = false;
    noCulling = false;
    noSorting = false;
    noLighting = false;

    delete[] textures;
    textures = NULL;
    textureCount = 0;

    delete[] shaders;
    shaders = NULL;
    shaderCount = 0;

    referenced = false;

    legacyIndex = -1;

    return;
}


MagnumBZMaterial::MagnumBZMaterial()
{
    textures = NULL;
    shaders = NULL;
    reset();
    return;
}


MagnumBZMaterial::~MagnumBZMaterial()
{
    delete[] textures;
    delete[] shaders;
    return;
}


MagnumBZMaterial::MagnumBZMaterial(const MagnumBZMaterial& m)
{
    textures = NULL;
    shaders = NULL;
    *this = m;
    return;
}


MagnumBZMaterial& MagnumBZMaterial::operator=(const MagnumBZMaterial& m)
{
    int i;

    referenced = false;

    name = m.name;
    aliases = m.aliases;

    dynamicColor = m.dynamicColor;
    memcpy (ambient, m.ambient, sizeof(ambient));
    memcpy (diffuse, m.diffuse, sizeof(diffuse));
    memcpy (specular, m.specular, sizeof(specular));
    memcpy (emission, m.emission, sizeof(emission));
    shininess = m.shininess;
    alphaThreshold = m.alphaThreshold;
    occluder = m.occluder;
    groupAlpha = m.groupAlpha;
    noRadar = m.noRadar;
    noShadow = m.noShadow;
    noCulling = m.noCulling;
    noSorting = m.noSorting;
    noLighting = m.noLighting;

    delete[] textures;
    textureCount = m.textureCount;
    if (textureCount > 0)
        textures = new TextureInfo[textureCount];
    else
        textures = NULL;
    for (i = 0; i < textureCount; i++)
        textures[i] = m.textures[i];

    delete[] shaders;
    shaderCount = m.shaderCount;
    if (shaderCount > 0)
        shaders = new ShaderInfo[shaderCount];
    else
        shaders = NULL;
    for (i = 0; i < shaderCount; i++)
        shaders[i] = m.shaders[i];

    legacyIndex = m.legacyIndex;

    return *this;
}


bool MagnumBZMaterial::operator==(const MagnumBZMaterial& m) const
{
    int i;

    if ((dynamicColor != m.dynamicColor) ||
            (memcmp (ambient, m.ambient, sizeof(float[4])) != 0) ||
            (memcmp (diffuse, m.diffuse, sizeof(float[4])) != 0) ||
            (memcmp (specular, m.specular, sizeof(float[4])) != 0) ||
            (memcmp (emission, m.emission, sizeof(float[4])) != 0) ||
            (shininess != m.shininess) || (alphaThreshold != m.alphaThreshold) ||
            (occluder != m.occluder) || (groupAlpha != m.groupAlpha) ||
            (noRadar != m.noRadar) || (noShadow != m.noShadow) ||
            (noCulling != m.noCulling) || (noSorting != m.noSorting) ||
            (noLighting != m.noLighting))
        return false;

    if (textureCount != m.textureCount)
        return false;
    for (i = 0; i < textureCount; i++)
    {
        if ((textures[i].name != m.textures[i].name) ||
                (textures[i].matrix != m.textures[i].matrix) ||
                (textures[i].combineMode != m.textures[i].combineMode) ||
                (textures[i].useAlpha != m.textures[i].useAlpha) ||
                (textures[i].useColor != m.textures[i].useColor) ||
                (textures[i].useSphereMap != m.textures[i].useSphereMap))
            return false;
    }

    if (shaderCount != m.shaderCount)
        return false;
    for (i = 0; i < shaderCount; i++)
    {
        if (shaders[i].name != m.shaders[i].name)
            return false;
    }

    if (legacyIndex != m.legacyIndex)
        return false;

    return true;
}


static void* pack4Float(void *buf, const float values[4])
{
    int i;
    for (i = 0; i < 4; i++)
        buf = nboPackFloat(buf, values[i]);
    return buf;
}


static const void* unpack4Float(const void *buf, float values[4])
{
    int i;
    for (i = 0; i < 4; i++)
        buf = nboUnpackFloat(buf, values[i]);
    return buf;
}


void* MagnumBZMaterial::pack(void* buf) const
{
    int i;

    buf = nboPackStdString(buf, name);

    uint8_t modeByte = 0;
    if (noCulling)    modeByte |= (1 << 0);
    if (noSorting)    modeByte |= (1 << 1);
    if (noRadar)      modeByte |= (1 << 2);
    if (noShadow)     modeByte |= (1 << 3);
    if (occluder)  modeByte |= (1 << 4);
    if (groupAlpha)       modeByte |= (1 << 5);
    if (noLighting)       modeByte |= (1 << 6);
    buf = nboPackUByte(buf, modeByte);

    buf = nboPackInt(buf, dynamicColor);
    buf = pack4Float(buf, ambient);
    buf = pack4Float(buf, diffuse);
    buf = pack4Float(buf, specular);
    buf = pack4Float(buf, emission);
    buf = nboPackFloat(buf, shininess);
    buf = nboPackFloat(buf, alphaThreshold);

    buf = nboPackUByte(buf, textureCount);
    for (i = 0; i < textureCount; i++)
    {
        const TextureInfo* texinfo = &textures[i];

        buf = nboPackStdString(buf, texinfo->name);
        buf = nboPackInt(buf, texinfo->matrix);
        buf = nboPackInt(buf, texinfo->combineMode);
        unsigned char stateByte = 0;
        if (texinfo->useAlpha)
            stateByte = stateByte | (1 << 0);
        if (texinfo->useColor)
            stateByte = stateByte | (1 << 1);
        if (texinfo->useSphereMap)
            stateByte = stateByte | (1 << 2);
        buf = nboPackUByte(buf, stateByte);
    }

    buf = nboPackUByte(buf, shaderCount);
    for (i = 0; i < shaderCount; i++)
        buf = nboPackStdString(buf, shaders[i].name);

    return buf;
}


const void* MagnumBZMaterial::unpack(const void* buf)
{
    int i;
    int32_t inTmp;

    buf = nboUnpackStdString(buf, name);

    uint8_t modeByte;
    buf = nboUnpackUByte(buf, modeByte);
    noCulling = (modeByte & (1 << 0)) != 0;
    noSorting = (modeByte & (1 << 1)) != 0;
    noRadar   = (modeByte & (1 << 2)) != 0;
    noShadow  = (modeByte & (1 << 3)) != 0;
    occluder  = (modeByte & (1 << 4)) != 0;
    groupAlpha    = (modeByte & (1 << 5)) != 0;
    noLighting    = (modeByte & (1 << 6)) != 0;

    buf = nboUnpackInt(buf, inTmp);
    dynamicColor = int(inTmp);
    buf = unpack4Float(buf, ambient);
    buf = unpack4Float(buf, diffuse);
    buf = unpack4Float(buf, specular);
    buf = unpack4Float(buf, emission);
    buf = nboUnpackFloat(buf, shininess);
    buf = nboUnpackFloat(buf, alphaThreshold);

    unsigned char tCount;
    buf = nboUnpackUByte(buf, tCount);
    textureCount = tCount;
    textures = new TextureInfo[textureCount];
    for (i = 0; i < textureCount; i++)
    {
        TextureInfo* texinfo = &textures[i];
        buf = nboUnpackStdString(buf, texinfo->name);
        texinfo->localname = texinfo->name;
        buf = nboUnpackInt(buf, inTmp);
        texinfo->matrix = int(inTmp);
        buf = nboUnpackInt(buf, inTmp);
        texinfo->combineMode = int(inTmp);
        texinfo->useAlpha = false;
        texinfo->useColor = false;
        texinfo->useSphereMap = false;
        unsigned char stateByte;
        buf = nboUnpackUByte(buf, stateByte);
        if (stateByte & (1 << 0))
            texinfo->useAlpha = true;
        if (stateByte & (1 << 1))
            texinfo->useColor = true;
        if (stateByte & (1 << 2))
            texinfo->useSphereMap = true;
    }

    unsigned char sCount;
    buf = nboUnpackUByte(buf, sCount);
    shaderCount = sCount;
    shaders = new ShaderInfo[shaderCount];
    for (i = 0; i < shaderCount; i++)
        buf = nboUnpackStdString(buf, shaders[i].name);

    return buf;
}


int MagnumBZMaterial::packSize() const
{
    int i;

    const int modeSize = sizeof(uint8_t);

    const int colorSize = sizeof(int32_t) + (4 * sizeof(float[4])) +
                          sizeof(float) + sizeof(float);

    int textureSize = sizeof(unsigned char);
    for (i = 0; i < textureCount; i++)
    {
        textureSize += nboStdStringPackSize(textures[i].name);
        textureSize += sizeof(int32_t);
        textureSize += sizeof(int32_t);
        textureSize += sizeof(unsigned char);
    }

    int shaderSize = sizeof(unsigned char);
    for (i = 0; i < shaderCount; i++)
        shaderSize += nboStdStringPackSize(shaders[i].name);

    return nboStdStringPackSize(name) + modeSize + colorSize + textureSize + shaderSize;
}


static void printColor(std::ostream& out, const char *name,
                       const float color[4], const float reference[4])
{
    if (memcmp(color, reference, sizeof(float[4])) != 0)
    {
        out << name << color[0] << " " << color[1] << " "
            << color[2] << " " << color[3] << std::endl;
    }
    return;
}


void MagnumBZMaterial::print(std::ostream& out, const std::string& indent) const
{
    int i;

    out << indent << "material" << std::endl;

    if (name.size() > 0)
        out << indent << "  name " << name << std::endl;

    if (dynamicColor != defaultMaterial.dynamicColor)
    {
        out << indent << "  dyncol ";
        const DynamicColor* dyncol = DYNCOLORMGR.getColor(dynamicColor);
        if ((dyncol != NULL) && (dyncol->getName().size() > 0))
            out << dyncol->getName();
        else
            out << dynamicColor;
        out << std::endl;
    }
    printColor(out, "  ambient ",  ambient,  defaultMaterial.ambient);
    printColor(out, "  diffuse ",  diffuse,  defaultMaterial.diffuse);
    printColor(out, "  specular ", specular, defaultMaterial.specular);
    printColor(out, "  emission ", emission, defaultMaterial.emission);
    if (shininess != defaultMaterial.shininess)
        out << indent << "  shininess " << shininess << std::endl;
    if (alphaThreshold != defaultMaterial.alphaThreshold)
        out << indent << "  alphathresh " << alphaThreshold << std::endl;
    if (occluder)
        out << indent << "  occluder" << std::endl;
    if (groupAlpha)
        out << indent << "  groupAlpha" << std::endl;
    if (noRadar)
        out << indent << "  noradar" << std::endl;
    if (noShadow)
        out << indent << "  noshadow" << std::endl;
    if (noCulling)
        out << indent << "  noculling" << std::endl;
    if (noSorting)
        out << indent << "  nosorting" << std::endl;
    if (noLighting)
        out << indent << "  nolighting" << std::endl;

    for (i = 0; i < textureCount; i++)
    {
        const TextureInfo* texinfo = &textures[i];
        out << indent << "  addtexture " << texinfo->name << std::endl;
        if (texinfo->matrix != -1)
        {
            out << indent << "    texmat ";
            const TextureMatrix* texmat = TEXMATRIXMGR.getMatrix(texinfo->matrix);
            if ((texmat != NULL) && (texmat->getName().size() > 0))
                out << texmat->getName();
            else
                out << texinfo->matrix;
            out << std::endl;
        }

        if (!texinfo->useAlpha)
            out << indent << "    notexalpha" << std::endl;
        if (!texinfo->useColor)
            out << indent << "    notexcolor" << std::endl;
        if (texinfo->useSphereMap)
            out << indent << "    spheremap" << std::endl;
    }

    for (i = 0; i < shaderCount; i++)
    {
        const ShaderInfo* shdinfo = &shaders[i];
        out << indent << "  addshader " << shdinfo->name << std::endl;
    }

    out << indent << "end" << std::endl << std::endl;

    return;
}


void MagnumBZMaterial::printMTL(std::ostream& out, const std::string& UNUSED(indent)) const
{
    out << "newmtl ";
    if (name.size() > 0)
        out << name << std::endl;
    else
        out << MAGNUMMATERIALMGR.getIndex(this) << std::endl;
    if (noLighting)
        out << "illum 0" << std::endl;
    else
        out << "illum 2" << std::endl;
    out << "d " << diffuse[3] << std::endl;
    const float* c;
    c = ambient; // not really used
    out << "#Ka " << c[0] << " " << c[1] << " " << c[2] << std::endl;
    c = diffuse;
    out << "Kd " << c[0] << " " << c[1] << " " << c[2] << std::endl;
    c = emission;
    out << "Ke " << c[0] << " " << c[1] << " " << c[2] << std::endl;
    c = specular;
    out << "Ks " << c[0] << " " << c[1] << " " << c[2] << std::endl;
    out << "Ns " << (1000.0f * (shininess / 128.0f)) << std::endl;
    if (textureCount > 0)
    {
        const TextureInfo* ti = &textures[0];
        const unsigned int nlen = (unsigned int)ti->name.size();
        if (nlen > 0)
        {
            std::string texname = ti->name;
            const char* cname = texname.c_str();
            if ((nlen < 4) || (strcasecmp(cname + (nlen - 4), ".png") != 0))
                texname += ".png";
            out << "map_Kd " << texname << std::endl;
        }
    }
    out << std::endl;
    return;
}


/****************************************************************************/
//
// Parameter setting
//

bool MagnumBZMaterial::setName(const std::string& matname)
{
    if (matname.size() <= 0)
    {
        name = "";
        return false;
    }
    else if ((matname[0] >= '0') && (matname[0] <= '9'))
    {
        name = "";
        return false;
    }
    else
        name = matname;
    return true;
}

bool MagnumBZMaterial::addAlias(const std::string& alias)
{
    if (alias.size() <= 0)
    {
        name = "";
        return false;
    }
    else if ((alias[0] >= '0') && (alias[0] <= '9'))
    {
        name = "";
        return false;
    }
    else
    {
        for ( unsigned int i = 0; i < (unsigned int)aliases.size(); i++)
        {
            if ( aliases[i] == alias )
                return true;
        }
        aliases.push_back(alias); // only add it if it's new
    }
    return true;
}

void MagnumBZMaterial::setDynamicColor(int dyncol)
{
    dynamicColor = dyncol;
    return;
}

void MagnumBZMaterial::setAmbient(const float color[4])
{
    memcpy (ambient, color, sizeof(float[4]));
    return;
}

void MagnumBZMaterial::setDiffuse(const float color[4])
{
    memcpy (diffuse, color, sizeof(float[4]));
    return;
}

void MagnumBZMaterial::setSpecular(const float color[4])
{
    memcpy (specular, color, sizeof(float[4]));
    return;
}

void MagnumBZMaterial::setEmission(const float color[4])
{
    memcpy (emission, color, sizeof(float[4]));
    return;
}

void MagnumBZMaterial::setShininess(const float shine)
{
    shininess = shine;
    return;
}

void MagnumBZMaterial::setAlphaThreshold(const float thresh)
{
    alphaThreshold = thresh;
    return;
}

void MagnumBZMaterial::setOccluder(bool value)
{
    occluder = value;
    return;
}

void MagnumBZMaterial::setGroupAlpha(bool value)
{
    groupAlpha = value;
    return;
}

void MagnumBZMaterial::setNoRadar(bool value)
{
    noRadar = value;
    return;
}

void MagnumBZMaterial::setNoShadow(bool value)
{
    noShadow = value;
    return;
}

void MagnumBZMaterial::setNoCulling(bool value)
{
    noCulling = value;
    return;
}

void MagnumBZMaterial::setNoSorting(bool value)
{
    noSorting = value;
    return;
}

void MagnumBZMaterial::setNoLighting(bool value)
{
    noLighting = value;
    return;
}


void MagnumBZMaterial::addTexture(const std::string& texname)
{
    textureCount++;
    TextureInfo* tmpinfo = new TextureInfo[textureCount];
    for (int i = 0; i < (textureCount - 1); i++)
        tmpinfo[i] = textures[i];
    delete[] textures;
    textures = tmpinfo;

    TextureInfo* texinfo = &textures[textureCount - 1];
    texinfo->name = texname;
    texinfo->localname = texname;
    texinfo->matrix = -1;
    texinfo->combineMode = decal;
    texinfo->useAlpha = false;
    texinfo->useColor = true;
    texinfo->useSphereMap = false;

    auto t = MagnumTextureManager::instance().getTexture(texname.c_str());
    if (t.texture) {
        texinfo->useAlpha = t.hasAlpha;
    }

    return;
}

void MagnumBZMaterial::setTexture(const std::string& texname)
{
    if (textureCount <= 0)
        addTexture(texname);
    else
        textures[textureCount - 1].name = texname;

    return;
}

void MagnumBZMaterial::setTextureLocal(int texid, const std::string& localname)
{
    if ((texid >= 0) && (texid < textureCount))
        textures[texid].localname = localname;
    return;
}

void MagnumBZMaterial::setTextureMatrix(int matrix)
{
    if (textureCount > 0)
        textures[textureCount - 1].matrix = matrix;
    return;
}

void MagnumBZMaterial::setCombineMode(int mode)
{
    if (textureCount > 0)
        textures[textureCount - 1].combineMode = mode;
    return;
}

void MagnumBZMaterial::setUseTextureAlpha(bool value)
{
    if (textureCount > 0)
        textures[textureCount - 1].useAlpha = value;
    return;
}

void MagnumBZMaterial::setUseColorOnTexture(bool value)
{
    if (textureCount > 0)
        textures[textureCount - 1].useColor = value;
    return;
}

void MagnumBZMaterial::setUseSphereMap(bool value)
{
    if (textureCount > 0)
        textures[textureCount - 1].useSphereMap = value;
    return;
}


void MagnumBZMaterial::clearTextures()
{
    delete[] textures;
    textures = NULL;
    textureCount = 0;
    return;
}


void MagnumBZMaterial::setShader(const std::string& shadername)
{
    if (shaderCount <= 0)
        addShader(shadername);
    else
        shaders[shaderCount - 1].name = shadername;

    return;
}

void MagnumBZMaterial::addShader(const std::string& shaderName)
{
    shaderCount++;
    ShaderInfo* tmpinfo = new ShaderInfo[shaderCount];
    for (int i = 0; i < (shaderCount - 1); i++)
        tmpinfo[i] = shaders[i];
    delete[] shaders;
    shaders = tmpinfo;
    shaders[shaderCount - 1].name = shaderName;
    return;
}


void MagnumBZMaterial::clearShaders()
{
    delete[] shaders;
    shaders = NULL;
    shaderCount = 0;
    return;
}

void MagnumBZMaterial::setLegacyIndex(int ind) {
    legacyIndex = ind;
}


/****************************************************************************/
//
// Parameter retrieval
//

const std::string& MagnumBZMaterial::getName() const
{
    return name;
}

const std::vector<std::string>& MagnumBZMaterial::getAliases() const
{
    return aliases;
}

int MagnumBZMaterial::getDynamicColor() const
{
    return dynamicColor;
}

const float* MagnumBZMaterial::getAmbient() const
{
    return ambient;
}

const float* MagnumBZMaterial::getDiffuse() const
{
    return diffuse;
}

const float* MagnumBZMaterial::getSpecular() const
{
    return specular;
}

const float* MagnumBZMaterial::getEmission() const
{
    return emission;
}

float MagnumBZMaterial::getShininess() const
{
    return shininess;
}

float MagnumBZMaterial::getAlphaThreshold() const
{
    return alphaThreshold;
}

bool MagnumBZMaterial::getOccluder() const
{
    return occluder;
}

bool MagnumBZMaterial::getGroupAlpha() const
{
    return groupAlpha;
}

bool MagnumBZMaterial::getNoRadar() const
{
    return noRadar;
}

bool MagnumBZMaterial::getNoShadow() const
{
    return noShadow;
}

bool MagnumBZMaterial::getNoCulling() const
{
    return noCulling;
}

bool MagnumBZMaterial::getNoSorting() const
{
    return noSorting;
}

bool MagnumBZMaterial::getNoLighting() const
{
    return noLighting;
}


int MagnumBZMaterial::getTextureCount() const
{
    return textureCount;
}

const std::string& MagnumBZMaterial::getTexture(int texid) const
{
    if ((texid >= 0) && (texid < textureCount))
        return textures[texid].name;
    else
        return nullString;
}

const std::string& MagnumBZMaterial::getTextureLocal(int texid) const
{
    if ((texid >= 0) && (texid < textureCount))
        return textures[texid].localname;
    else
        return nullString;
}

int MagnumBZMaterial::getTextureMatrix(int texid) const
{
    if ((texid >= 0) && (texid < textureCount))
        return textures[texid].matrix;
    else
        return -1;
}

int MagnumBZMaterial::getCombineMode(int texid) const
{
    if ((texid >= 0) && (texid < textureCount))
        return textures[texid].combineMode;
    else
        return -1;
}

bool MagnumBZMaterial::getUseTextureAlpha(int texid) const
{
    if ((texid >= 0) && (texid < textureCount))
        return textures[texid].useAlpha;
    else
        return false;
}

bool MagnumBZMaterial::getUseColorOnTexture(int texid) const
{
    if ((texid >= 0) && (texid < textureCount))
        return textures[texid].useColor;
    else
        return false;
}

bool MagnumBZMaterial::getUseSphereMap(int texid) const
{
    if ((texid >= 0) && (texid < textureCount))
        return textures[texid].useSphereMap;
    else
        return false;
}


int MagnumBZMaterial::getShaderCount() const
{
    return shaderCount;
}

const std::string& MagnumBZMaterial::getShader(int shdid) const
{
    if ((shdid >= 0) && (shdid < shaderCount))
        return shaders[shdid].name;
    else
        return nullString;
}


bool MagnumBZMaterial::isInvisible() const
{
    const DynamicColor* dyncol = DYNCOLORMGR.getColor(dynamicColor);
    if ((diffuse[3] == 0.0f) && (dyncol == NULL) &&
            !((textureCount > 0) && !textures[0].useColor))
        return true;
    return false;
}

int MagnumBZMaterial::getLegacyIndex() const {
    return legacyIndex;
}


// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
