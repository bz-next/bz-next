#ifndef MAGNUMBZMATERIAL_H
#define MAGNUMBZMATERIAL_H


#include "common.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>



class MagnumBZMaterial;
typedef std::set<const MagnumBZMaterial*> MagnumMaterialSet;
typedef std::map<const MagnumBZMaterial*,
        const MagnumBZMaterial*> MagnumMaterialMap;


class MagnumBZMaterial
{
public:
    MagnumBZMaterial();
    MagnumBZMaterial(const MagnumBZMaterial& material);
    ~MagnumBZMaterial();

    bool operator==(const MagnumBZMaterial& material) const;
    MagnumBZMaterial& operator=(const MagnumBZMaterial& material);

    void reset();

    void setReference() const;  // const exploits mutable "referenced" variable below
    bool getReference() const;

    //
    // Parameter setting
    //

    bool setName(const std::string&);
    bool addAlias(const std::string&);

    void setDynamicColor(int);
    void setAmbient(const float[4]);
    void setDiffuse(const float[4]);
    void setSpecular(const float[4]);
    void setEmission(const float[4]);
    void setShininess(const float);

    void setOccluder(bool);
    void setGroupAlpha(bool);
    void setNoLighting(bool);
    void setNoRadar(bool);
    void setNoShadow(bool);
    void setNoCulling(bool);
    void setNoSorting(bool);
    void setAlphaThreshold(const float);

    // the following set()'s operate on the last added texture
    void addTexture(const std::string&);
    void setTexture(const std::string&);
    void setTextureLocal(int texid, const std::string& localname);
    void setTextureMatrix(int);
    void setCombineMode(int);
    void setUseTextureAlpha(bool);
    void setUseColorOnTexture(bool);
    void setUseSphereMap(bool);
    void clearTextures(); // remove all textures

    void addShader(const std::string&);
    void setShader(const std::string&);
    void clearShaders(); // remove all shaders

    //
    // Parameter getting
    //

    const std::string& getName() const;
    const std::vector<std::string>& getAliases() const;

    int getDynamicColor() const;
    const float* getAmbient() const;
    const float* getDiffuse() const;
    const float* getSpecular() const;
    const float* getEmission() const;
    float getShininess() const;

    bool getOccluder() const;
    bool getGroupAlpha() const;
    bool getNoRadar() const;
    bool getNoShadow() const;
    bool getNoCulling() const;
    bool getNoSorting() const;
    bool getNoLighting() const;
    float getAlphaThreshold() const;

    int getTextureCount() const;
    const std::string& getTexture(int) const;
    const std::string& getTextureLocal(int) const;
    int getTextureMatrix(int) const;
    int getCombineMode(int) const;
    bool getUseTextureAlpha(int) const;
    bool getUseColorOnTexture(int) const;
    bool getUseSphereMap(int) const;

    int getShaderCount() const;
    const std::string& getShader(int) const;

    //
    // Utilities
    //

    bool isInvisible() const;

    int packSize() const;
    void *pack(void *) const;
    const void *unpack(const void *);

    void print(std::ostream& out, const std::string& indent) const;
    void printMTL(std::ostream& out, const std::string& indent) const;

    static const MagnumBZMaterial* getDefault();

    void setLegacyIndex(int ind);
    int getLegacyIndex() const;

    // data
private:
    std::string name;
    std::vector<std::string> aliases;

    mutable bool referenced;

    int dynamicColor;
    float ambient[4];
    float diffuse[4];
    float specular[4];
    float emission[4];
    float shininess;

    bool occluder;
    bool groupAlpha;
    bool noRadar;
    bool noShadow;
    bool noCulling;
    bool noSorting;
    bool noLighting;
    float alphaThreshold;

    enum CombineModes
    {
        replace = 0,
        modulate,
        decal,
        blend,
        add,
        combine
    };
    int textureCount;
    typedef struct
    {
        std::string name;
        std::string localname;
        int matrix;
        int combineMode;
        bool useAlpha;
        bool useColor;
        bool useSphereMap;
    } TextureInfo;
    TextureInfo* textures;

    int shaderCount;
    typedef struct
    {
        std::string name;
    } ShaderInfo;
    ShaderInfo* shaders;

    // For interfacing with legacy code that references material indices
    int legacyIndex;

private:
    static std::string nullString;
    static MagnumBZMaterial defaultMaterial;
};

inline const MagnumBZMaterial* MagnumBZMaterial::getDefault()
{
    return &defaultMaterial;
}

inline void MagnumBZMaterial::setReference() const
{
    referenced = true;
    return;
}

inline bool MagnumBZMaterial::getReference() const
{
    return referenced;
}


class MagnumBZMaterialManager
{
public:
    MagnumBZMaterialManager();
    ~MagnumBZMaterialManager();

    void loadDefaultMaterials();

    void update();
    void clear(bool loadDefaults = true);
    const MagnumBZMaterial* addMaterial(const MagnumBZMaterial* material);
    const MagnumBZMaterial* addLegacyIndexedMaterial(const MagnumBZMaterial* material);
    const MagnumBZMaterial* findMaterial(const std::string& name) const;
    const MagnumBZMaterial* getMaterial(int id) const;
    // Legacy function for adapting some deep parts of mesh code that references material indices
    int getIndex(const MagnumBZMaterial* material) const;

    // Trigger the texture manager to load all referenced textures by accessing them
    void forceLoadTextures();

    std::vector<std::string> getMaterialNames();

    typedef std::set<std::string> TextureSet;
    void makeTextureList(TextureSet& set, bool referenced) const;
    void setTextureLocal(const std::string& url, const std::string& local);

    void* pack(void*);
    const void* unpack(const void*);
    int packSize();

    void print(std::ostream& out, const std::string& indent) const;
    void printMTL(std::ostream& out, const std::string& indent) const;
    void printReference(std::ostream& out, const MagnumBZMaterial* mat) const;

private:
    std::vector<MagnumBZMaterial*> materials;
    int legacyMaterialIndex;
    int unnamedCount;   // Auto generate names for unnamed or duplicate named materials
    int duplicateNameCount;
    int unnamedAliasCount;
};


extern MagnumBZMaterialManager MAGNUMMATERIALMGR;

#endif
