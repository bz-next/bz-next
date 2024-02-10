#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/Copy.h>
#include <Magnum/Math/Vector.h>
#include <Magnum/Math/Matrix4.h>

#include "Corrade/Containers/ArrayView.h"
#include "IndexedMeshData.h"
#include "Magnum/GL/GL.h"
#include "Magnum/Math/Vector3.h"
#include "Magnum/MeshTools/Compile.h"
#include "Magnum/MeshTools/CompressIndices.h"
#include "Magnum/Shaders/PhongGL.h"
#include "Magnum/Trade/Data.h"
#include "Magnum/Trade/MeshData.h"


#include "Magnum/Trade/Trade.h"
#include "Magnum/Types.h"
#include "MagnumTextureManager.h"
#include "MeshObstacle.h"
#include "WorldPrimitiveGenerator.h"
#include "MagnumBZMaterial.h"

#include "StateDatabase.h"
#include "WorldMeshGenerator.h"
#include "BoxBuilding.h"
#include "BaseBuilding.h"
#include "Team.h"
#include "MeshDrawInfo.h"
#include "BZDBCache.h"
#include "DynamicColor.h"

#include <vector>
#include <utility>
#include <set>

using namespace Magnum;
using namespace Corrade;

void WorldObject::addMatMesh(std::string materialname, IndexedMeshData&& md) {
    matMeshes.emplace_back(std::make_pair(materialname, std::move(md)));
}

const std::vector<MatMesh>& WorldObject::getMatMeshes() const {
    return matMeshes;
}

void WorldMeshGenerator::addBox(BoxBuilding& o) {
    // The old code mapped textures straight to the box without a material
    // Instead, we assume we have materials called boxWallMaterial and
    // boxTopMaterial loaded earlier when initializing the program
    const MagnumBZMaterial *boxWallMat = MAGNUMMATERIALMGR.findMaterial("boxWallMaterial");
    const MagnumBZMaterial *boxTopMat = MAGNUMMATERIALMGR.findMaterial("boxTopMaterial");

    auto &tm = MagnumTextureManager::instance();

    float texFactor = BZDB.eval("boxWallTexRepeat");

    WorldObject boxObj;

    // Annoyingly, boxes don't use a TextureMatrix to scale the texture...
    // instead the UV coordinates are fudged. We will do this for now,
    // but in the future the box material should just be updated with a
    // TextureMatrix
    // TODO: Add a texmat to box materials and remove this!

    TextureData bwtex = {NULL, 0, 0};

    if (o.userTextures[0].size())
        bwtex = tm.getTexture(o.userTextures[0].c_str());
    if (bwtex.texture == NULL)
        bwtex = MagnumTextureManager::instance().getTexture(BZDB.get("boxWallTexture").c_str());

    float boxTexWidth, boxTexHeight;
    boxTexWidth = boxTexHeight = 0.2f * BZDB.eval(StateDatabase::BZDB_BOXHEIGHT);
    if (bwtex.texture)
        boxTexWidth = (float)bwtex.width / (float)bwtex.height * boxTexHeight;


    float base[3], sCorner[3], tCorner[3];
    float sEdge[3], tEdge[3];
    float uRepeats, vRepeats;

    auto computeEdges =
        [&base, &sCorner, &tCorner, &sEdge, &tEdge, &uRepeats, &vRepeats, texFactor, boxTexWidth, boxTexHeight]
        (const BoxBuilding& bb, int faceNum) {
        switch (faceNum) {
        case 1:
            bb.getCorner(0, base);
            bb.getCorner(1, sCorner);
            bb.getCorner(4, tCorner);
            break;
        case 2:
            bb.getCorner(1, base);
            bb.getCorner(2, sCorner);
            bb.getCorner(5, tCorner);
            break;
        case 3:
            bb.getCorner(2, base);
            bb.getCorner(3, sCorner);
            bb.getCorner(6, tCorner);
            break;
        case 4:
            bb.getCorner(3, base);
            bb.getCorner(0, sCorner);
            bb.getCorner(7, tCorner);
            break;
        case 5:                         //This is the top polygon
            bb.getCorner(4, base);
            bb.getCorner(5, sCorner);
            bb.getCorner(7, tCorner);
            break;
        case 6:                         //This is the bottom polygon
            //Don't generate the bottom polygon if on the ground (or lower)
            if (bb.getPosition()[2] > 0.0f)
            {
                bb.getCorner(0, base);
                bb.getCorner(3, sCorner);
                bb.getCorner(1, tCorner);
                break;
            }
            return false;
        }
        sEdge[0] = sCorner[0] - base[0];
        sEdge[1] = sCorner[1] - base[1];
        sEdge[2] = sCorner[2] - base[2];
        tEdge[0] = tCorner[0] - base[0];
        tEdge[1] = tCorner[1] - base[1];
        tEdge[2] = tCorner[2] - base[2];
        // Magic factor of 3.3 seems to make this match the game
        if (faceNum <= 4) {
            uRepeats = -texFactor*boxTexWidth*3.3;
            vRepeats = -texFactor*boxTexWidth*3.3;
        } else {
            uRepeats = -boxTexHeight*3.3;
            vRepeats = -boxTexHeight*3.3;
        }
        const float sLength = sqrtf(float(sEdge[0] * sEdge[0] +
                                      sEdge[1] * sEdge[1] + sEdge[2] * sEdge[2]));
        const float tLength = sqrtf(float(tEdge[0] * tEdge[0] +
                                        tEdge[1] * tEdge[1] + tEdge[2] * tEdge[2]));
        if (uRepeats < 0.0f) {
            uRepeats = - sLength / uRepeats;
        }
        if (vRepeats < 0.0f) {
            vRepeats = - tLength / vRepeats;
        }
        return true;
    };



    for (int i = 1; i <= 6; ++i) {
        if (computeEdges(o, i)) {
            if (i <= 4)
                boxObj.addMatMesh(
                    "boxWallMaterial",
                    WorldPrimitiveGenerator::quad(base, sEdge, tEdge, 0.0f, 0.0f, uRepeats, vRepeats));
            else
                boxObj.addMatMesh(
                    "boxTopMaterial",
                    WorldPrimitiveGenerator::quad(base, sEdge, tEdge, 0.0f, 0.0f, uRepeats, vRepeats));
        }
    }
    worldObjects.emplace_back(std::move(boxObj));
}

void WorldMeshGenerator::addPyr(PyramidBuilding& o) {
    WorldObject pyrObj;
    
    float texFactor = BZDB.eval("pyrWallTexRepeat");

    // The original pyramid code uses the box texture size as a magic texture scaling parameter...
    // needless to say, this should be changed
    float boxTexHeight = 0.2f * BZDB.eval(StateDatabase::BZDB_BOXHEIGHT);

    float base[3], sCorner[3], tCorner[3];
    float sEdge[3], tEdge[3];
    float uRepeats, vRepeats;
    bool isQuad = false;

    auto computeEdges =
        [&base, &sCorner, &tCorner, &sEdge, &tEdge, &uRepeats, &vRepeats, texFactor, &isQuad, boxTexHeight]
        (const PyramidBuilding& pyramid, int faceNum) {
        if (pyramid.getZFlip()) {
            switch (faceNum) {
            case 1:
                pyramid.getCorner(4, base);
                pyramid.getCorner(1, sCorner);
                pyramid.getCorner(0, tCorner);
                isQuad = false;
                break;
            case 2:
                pyramid.getCorner(4, base);
                pyramid.getCorner(2, sCorner);
                pyramid.getCorner(1, tCorner);
                isQuad = false;
                break;
            case 3:
                pyramid.getCorner(4, base);
                pyramid.getCorner(3, sCorner);
                pyramid.getCorner(2, tCorner);
                isQuad = false;
                break;
            case 4:
                pyramid.getCorner(4, base);
                pyramid.getCorner(0, sCorner);
                pyramid.getCorner(3, tCorner);
                isQuad = false;
                break;
            case 5:
                pyramid.getCorner(0, base);
                pyramid.getCorner(1, sCorner);
                pyramid.getCorner(3, tCorner);
                isQuad = true;
                break;
            }
        } else {
            switch (faceNum) {
            case 1:
                pyramid.getCorner(0, base);
                pyramid.getCorner(1, sCorner);
                pyramid.getCorner(4, tCorner);
                isQuad = false;
                break;
            case 2:
                pyramid.getCorner(1, base);
                pyramid.getCorner(2, sCorner);
                pyramid.getCorner(4, tCorner);
                isQuad = false;
                break;
            case 3:
                pyramid.getCorner(2, base);
                pyramid.getCorner(3, sCorner);
                pyramid.getCorner(4, tCorner);
                isQuad = false;
                break;
            case 4:
                pyramid.getCorner(3, base);
                pyramid.getCorner(0, sCorner);
                pyramid.getCorner(4, tCorner);
                isQuad = false;
                break;
            case 5:
                if ((pyramid.getPosition()[2] > 0.0f))
                {
                    pyramid.getCorner(0, base);
                    pyramid.getCorner(3, sCorner);
                    pyramid.getCorner(1, tCorner);
                    isQuad = true;
                } else {
                    return false;
                }
                break;
            }
        }
        sEdge[0] = sCorner[0] - base[0];
        sEdge[1] = sCorner[1] - base[1];
        sEdge[2] = sCorner[2] - base[2];
        tEdge[0] = tCorner[0] - base[0];
        tEdge[1] = tCorner[1] - base[1];
        tEdge[2] = tCorner[2] - base[2];

        // Magic factor of 2.8 2*sqrt(2) ??? seems to make the texmap match the game
        uRepeats = -texFactor*boxTexHeight * 2.8;
        vRepeats = -texFactor*boxTexHeight * 2.8;

        const float sLength = sqrtf(float(sEdge[0] * sEdge[0] +
                                    sEdge[1] * sEdge[1] + sEdge[2] * sEdge[2]));
        const float tLength = sqrtf(float(tEdge[0] * tEdge[0] +
                                        tEdge[1] * tEdge[1] + tEdge[2] * tEdge[2]));
        if (uRepeats < 0.0f) {
            uRepeats = - sLength / uRepeats;
        }
        if (vRepeats < 0.0f) {
            vRepeats = - tLength / vRepeats;
        }
        return true;
    };

    for (int i = 1; i <= 5; ++i) {
        if (computeEdges(o, i)) {
            pyrObj.addMatMesh(
                "pyrWallMaterial",
                isQuad ?
                    WorldPrimitiveGenerator::quad(base, sEdge, tEdge, 0.0f, 0.0f, uRepeats, vRepeats) :
                    WorldPrimitiveGenerator::tri(base, sEdge, tEdge, 0.0f, 0.0f, uRepeats, vRepeats));
        }
    }
    worldObjects.emplace_back(std::move(pyrObj));
}

void WorldMeshGenerator::addBase(BaseBuilding& o) {
    // The old code mapped textures straight to the box without a material
    // Instead, we assume we have materials called boxWallMaterial and
    // boxTopMaterial loaded earlier when initializing the program
    const MagnumBZMaterial *boxWallMat = MAGNUMMATERIALMGR.findMaterial(Team::getImagePrefix((TeamColor)o.getTeam()) + "boxWallMaterial");
    const MagnumBZMaterial *boxTopMat = MAGNUMMATERIALMGR.findMaterial(Team::getImagePrefix((TeamColor)o.getTeam()) + "boxTopMaterial");

    auto &tm = MagnumTextureManager::instance();

    float texFactor = BZDB.eval("boxWallTexRepeat");

    WorldObject baseObj;

    TextureData bwtex = {NULL, 0, 0};

    // TODO: Actually use these if we have them...
    // This is a mess wrt the material system
    if (o.userTextures[0].size())
        bwtex = tm.getTexture(o.userTextures[0].c_str());
    if (bwtex.texture == NULL) {
        std::string teamBase = Team::getImagePrefix((TeamColor)o.getTeam());
        teamBase += BZDB.get("baseWallTexture");
        bwtex = tm.getTexture(teamBase.c_str());
    }
    if (bwtex.texture == NULL)
        bwtex = tm.getTexture(BZDB.get("boxWallTexture").c_str());

    TextureData bttex = {NULL, 0, 0};

    if (o.userTextures[1].size())
        bttex = tm.getTexture(o.userTextures[1].c_str());
    if (bwtex.texture == NULL) {
        std::string teamBase = Team::getImagePrefix((TeamColor)o.getTeam());
        teamBase += BZDB.get("baseTopTexture");
        bttex = tm.getTexture(teamBase.c_str());
    }


    const float height = o.getHeight() + o.getPosition()[2];

    float base[3], sCorner[3], tCorner[3];
    float sEdge[3], tEdge[3];
    float uRepeats, vRepeats;

    auto computeEdges =
        [&base, &sCorner, &tCorner, &sEdge, &tEdge, &uRepeats, &vRepeats, texFactor, height]
        (const BaseBuilding& bb, int faceNum) {
        if (faceNum > 1 && height == 0) return false;
        if (faceNum >= 6) return false;
        if (height == 0) {
            bb.getCorner(0, base);
            bb.getCorner(3, tCorner);
            bb.getCorner(1, sCorner);
        } else {
            switch (faceNum) {
            case 1: // top
                bb.getCorner(4, base);
                bb.getCorner(5, sCorner);
                bb.getCorner(7, tCorner);
                break;
            case 2:
                bb.getCorner(0, base);
                bb.getCorner(1, sCorner);
                bb.getCorner(4, tCorner);
                break;
            case 3:
                bb.getCorner(1, base);
                bb.getCorner(2, sCorner);
                bb.getCorner(5, tCorner);
                break;
            case 4:
                bb.getCorner(2, base);
                bb.getCorner(3, sCorner);
                bb.getCorner(6, tCorner);
                break;
            case 5:                         //This is the top polygon
                bb.getCorner(3, base);
                bb.getCorner(0, sCorner);
                bb.getCorner(7, tCorner);
                break;
            case 6:                         //This is the bottom polygon
                //Don't generate the bottom polygon if on the ground (or lower)
                if (bb.getPosition()[2] > 0.0f)
                {
                    bb.getCorner(0, base);
                    bb.getCorner(3, sCorner);
                    bb.getCorner(1, tCorner);
                    break;
                }
                return false;
            }
        }
        sEdge[0] = sCorner[0] - base[0];
        sEdge[1] = sCorner[1] - base[1];
        sEdge[2] = sCorner[2] - base[2];
        tEdge[0] = tCorner[0] - base[0];
        tEdge[1] = tCorner[1] - base[1];
        tEdge[2] = tCorner[2] - base[2];
        if (faceNum == 1 || faceNum == 6) {
            uRepeats = vRepeats = 1.0f;
        } else {
            uRepeats = bb.getBreadth();
            vRepeats = bb.getHeight();
        }
        const float sLength = sqrtf(float(sEdge[0] * sEdge[0] +
                                      sEdge[1] * sEdge[1] + sEdge[2] * sEdge[2]));
        const float tLength = sqrtf(float(tEdge[0] * tEdge[0] +
                                        tEdge[1] * tEdge[1] + tEdge[2] * tEdge[2]));
        if (uRepeats < 0.0f) {
            uRepeats = - sLength / uRepeats;
        }
        if (vRepeats < 0.0f) {
            vRepeats = - tLength / vRepeats;
        }
        return true;
    };

    for (int i = 1; i <= 6; ++i) {
        if (computeEdges(o, i)) {
            if (i == 1 || i == 6)
                baseObj.addMatMesh(
                    Team::getImagePrefix((TeamColor)o.getTeam()) + "baseTopMaterial",
                    WorldPrimitiveGenerator::quad(base, sEdge, tEdge, 0.0f, 0.0f, uRepeats, vRepeats));
            else
                baseObj.addMatMesh(
                    Team::getImagePrefix((TeamColor)o.getTeam()) + "baseWallMaterial",
                    WorldPrimitiveGenerator::quad(base, sEdge, tEdge, 0.0f, 0.0f, uRepeats, vRepeats));
        }
    }
    worldObjects.emplace_back(std::move(baseObj));
}

void WorldMeshGenerator::addWall(WallObstacle& o) {
    WorldObject wallObj;
    if (o.getHeight() <= 0.0f)
        return;

    const MagnumBZMaterial *wallMat = MAGNUMMATERIALMGR.findMaterial("wallMaterial");
    auto &tm = MagnumTextureManager::instance();


    // make styles -- first the outer wall
    auto wallTexture = tm.getTexture( "wall" );
    float wallTexWidth, wallTexHeight;
    wallTexWidth = wallTexHeight = 10.0f;
    if (wallTexture.texture != NULL)
        wallTexWidth = (float)wallTexture.width / (float)wallTexture.height * wallTexHeight;

    float base[3], sCorner[3], tCorner[3];
    float sEdge[3], tEdge[3];

    auto computeEdges =
        [&base, &sCorner, &tCorner, &sEdge, &tEdge]
        (const WallObstacle& wo, int faceNum) {
        const float* pos = wo.getPosition();
        const float c = cosf(wo.getRotation());
        const float s = sinf(wo.getRotation());
        const float h = wo.getBreadth();
        switch (faceNum) {
        case 1:
            base[0] = pos[0] + s * h;
            base[1] = pos[1] - c * h;
            base[2] = 0.0f;
            sEdge[0] = -2.0f * s * h;
            sEdge[1] = 2.0f * c * h;
            sEdge[2] = 0.0f;
            tEdge[0] = 0.0f;
            tEdge[1] = 0.0f;
            tEdge[2] = wo.getHeight();
            break;
        default:
            return false;
        }
        const float sLength = sqrtf(float(sEdge[0] * sEdge[0] +
                                      sEdge[1] * sEdge[1] + sEdge[2] * sEdge[2]));
        const float tLength = sqrtf(float(tEdge[0] * tEdge[0] +
                                        tEdge[1] * tEdge[1] + tEdge[2] * tEdge[2]));
        return true;
    };
    if (computeEdges(o, 1)) {
        wallObj.addMatMesh(
            "wallMaterial",
            WorldPrimitiveGenerator::quad(base, sEdge, tEdge, 0.0f, 0.0f, o.getBreadth() / wallTexWidth, o.getHeight() / wallTexHeight));
    }
    worldObjects.emplace_back(std::move(wallObj));
}

void WorldMeshGenerator::addTeleporter(const Teleporter& o) {
    static const float texCoords[][4][2] =
    {
        {{ 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.5f, 9.5f }, { 0.0f, 9.5f }},
        {{ 0.5f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 9.5f }, { 0.5f, 9.5f }},
        {{ 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.5f, 9.0f }, { 0.0f, 9.0f }},
        {{ 0.5f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 9.0f }, { 0.5f, 9.0f }},
        {{ 0.5f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 9.0f }, { 0.5f, 9.0f }},
        {{ 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.5f, 9.0f }, { 0.0f, 9.0f }},
        {{ 0.5f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 9.0f }, { 0.5f, 9.0f }},
        {{ 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.5f, 9.0f }, { 0.0f, 9.0f }},
        {{ 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.5f, 5.0f }, { 0.5f, 5.0f }},
        {{ 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.5f, 4.0f }, { 0.5f, 4.0f }},
        {{ 0.0f, 0.0f }, { 5.0f, 0.0f }, { 5.0f, 0.5f }, { 0.0f, 0.5f }},
        {{ 0.0f, 0.5f }, { 5.0f, 0.5f }, { 5.0f, 1.0f }, { 0.0f, 1.0f }}
    };
    WorldObject teleObj;

    auto &tm = MagnumTextureManager::instance();

    int numParts = o.isHorizontal() ? 18 : 14;

    float base[3], sCorner[3], tCorner[3];
    float sEdge[3], tEdge[3];
    float uRepeats, vRepeats;
    float u, v, uc, vc;

    auto computeEdges =
        [&base, &sCorner, &tCorner, &sEdge, &tEdge, &uRepeats, &vRepeats, &u, &v, &uc, &vc]
        (const Teleporter& tele, int faceNum) {
        const float *pos = tele.getPosition ();
        const float c = cosf (tele.getRotation ());
        const float s = sinf (tele.getRotation ());
        const float h = tele.getBreadth () - tele.getBorder ();
        const float b = 0.5f * tele.getBorder ();
        const float d = h + b;
        const float z = tele.getHeight () - tele.getBorder ();
        GLfloat x[2], y[2];
        x[0] = c;
        x[1] = s;
        y[0] = -s;
        y[1] = c;
        if (tele.isHorizontal()) return false; // Horizontal teleporters not implemented yet
        switch (faceNum) {
        case 1: // top
            base[0] = pos[0] + d * y[0] + b * x[0] + b * y[0];
            base[1] = pos[1] + d * y[1] + b * x[1] + b * y[1];
            base[2] = pos[2];
            sEdge[0] = -2.0f * b * x[0];
            sEdge[1] = -2.0f * b * x[1];
            sEdge[2] = 0.0f;
            tEdge[0] = 0.0f;
            tEdge[1] = 0.0f;
            tEdge[2] = z + 2.0f * b;
            break;
        case 2:
            base[0] = pos[0] - d * y[0] - b * x[0] - b * y[0];
            base[1] = pos[1] - d * y[1] - b * x[1] - b * y[1];
            base[2] = pos[2];
            sEdge[0] = 2.0f * b * x[0];
            sEdge[1] = 2.0f * b * x[1];
            sEdge[2] = 0.0f;
            tEdge[0] = 0.0f;
            tEdge[1] = 0.0f;
            tEdge[2] = z + 2.0f * b;
            break;
        case 3:
            base[0] = pos[0] + d * y[0] - b * x[0] - b * y[0];
            base[1] = pos[1] + d * y[1] - b * x[1] - b * y[1];
            base[2] = pos[2];
            sEdge[0] = 2.0f * b * x[0];
            sEdge[1] = 2.0f * b * x[1];
            sEdge[2] = 0.0f;
            tEdge[0] = 0.0f;
            tEdge[1] = 0.0f;
            tEdge[2] = z;
            break;
        case 4:
            base[0] = pos[0] - d * y[0] + b * x[0] + b * y[0];
            base[1] = pos[1] - d * y[1] + b * x[1] + b * y[1];
            base[2] = pos[2];
            sEdge[0] = -2.0f * b * x[0];
            sEdge[1] = -2.0f * b * x[1];
            sEdge[2] = 0.0f;
            tEdge[0] = 0.0f;
            tEdge[1] = 0.0f;
            tEdge[2] = z;
            break;
        case 5:                         //This is the top polygon
            base[0] = pos[0] + d * y[0] + b * x[0] - b * y[0];
            base[1] = pos[1] + d * y[1] + b * x[1] - b * y[1];
            base[2] = pos[2];
            sEdge[0] = 2.0f * b * y[0];
            sEdge[1] = 2.0f * b * y[1];
            sEdge[2] = 0.0f;
            tEdge[0] = 0.0f;
            tEdge[1] = 0.0f;
            tEdge[2] = z;
            break;
        case 6:                         //This is the bottom polygon
            base[0] = pos[0] - d * y[0] - b * x[0] + b * y[0];
            base[1] = pos[1] - d * y[1] - b * x[1] + b * y[1];
            base[2] = pos[2];
            sEdge[0] = -2.0f * b * y[0];
            sEdge[1] = -2.0f * b * y[1];
            sEdge[2] = 0.0f;
            tEdge[0] = 0.0f;
            tEdge[1] = 0.0f;
            tEdge[2] = z;
            break;
        case 7:
            base[0] = pos[0] + d * y[0] - b * x[0] + b * y[0];
            base[1] = pos[1] + d * y[1] - b * x[1] + b * y[1];
            base[2] = pos[2];
            sEdge[0] = -2.0f * b * y[0];
            sEdge[1] = -2.0f * b * y[1];
            sEdge[2] = 0.0f;
            tEdge[0] = 0.0f;
            tEdge[1] = 0.0f;
            tEdge[2] = z;
            break;
        case 8:
            base[0] = pos[0] - d * y[0] + b * x[0] - b * y[0];
            base[1] = pos[1] - d * y[1] + b * x[1] - b * y[1];
            base[2] = pos[2];
            sEdge[0] = 2.0f * b * y[0];
            sEdge[1] = 2.0f * b * y[1];
            sEdge[2] = 0.0f;
            tEdge[0] = 0.0f;
            tEdge[1] = 0.0f;
            tEdge[2] = z;
            break;
        case 9:
            base[0] = pos[0] - d * y[0] - b * x[0] - b * y[0];
            base[1] = pos[1] - d * y[1] - b * x[1] - b * y[1];
            base[2] = pos[2] + z + 2.0f * b;
            sEdge[0] = 2.0f * b * x[0];
            sEdge[1] = 2.0f * b * x[1];
            sEdge[2] = 0.0f;
            tEdge[0] = 2.0f * (d + b) * y[0];
            tEdge[1] = 2.0f * (d + b) * y[1];
            tEdge[2] = 0.0f;
            break;
        case 10:
            base[0] = pos[0] - d * y[0] + b * x[0] + b * y[0];
            base[1] = pos[1] - d * y[1] + b * x[1] + b * y[1];
            base[2] = pos[2] + z;
            sEdge[0] = -2.0f * b * x[0];
            sEdge[1] = -2.0f * b * x[1];
            sEdge[2] = 0.0f;
            tEdge[0] = 2.0f * (d - b) * y[0];
            tEdge[1] = 2.0f * (d - b) * y[1];
            tEdge[2] = 0.0f;
            break;
        case 11:
            base[0] = pos[0] - d * y[0] + b * x[0] - b * y[0];
            base[1] = pos[1] - d * y[1] + b * x[1] - b * y[1];
            base[2] = pos[2] + z;
            sEdge[0] = 2.0f * (d + b) * y[0];
            sEdge[1] = 2.0f * (d + b) * y[1];
            sEdge[2] = 0.0f;
            tEdge[0] = 0.0f;
            tEdge[1] = 0.0f;
            tEdge[2] = 2.0f * b;
            break;
        case 12:
            base[0] = pos[0] + d * y[0] - b * x[0] + b * y[0];
            base[1] = pos[1] + d * y[1] - b * x[1] + b * y[1];
            base[2] = pos[2] + z;
            sEdge[0] = -2.0f * (d + b) * y[0];
            sEdge[1] = -2.0f * (d + b) * y[1];
            sEdge[2] = 0.0f;
            tEdge[0] = 0.0f;
            tEdge[1] = 0.0f;
            tEdge[2] = 2.0f * b;
            break;
        default:
            return false;
        }
        if (faceNum >= 1 && faceNum <= 12)
        {
            u = texCoords[faceNum - 1][0][0];
            v = texCoords[faceNum - 1][0][1];
            uc = texCoords[faceNum - 1][1][0] - u;
            vc = texCoords[faceNum - 1][3][1] - v;
        }
        else
        {
            u = v = 0.0f;
            uc = vc = 1.0f;
        }
        return true;
    };
    for (int i = 1; i <= 12; ++i) {
        if (computeEdges(o, i)) {
            teleObj.addMatMesh(
                "cautionMaterial",
                WorldPrimitiveGenerator::quad(base, sEdge, tEdge, u, v, uc, vc));
        }
    }

    auto *bl = o.getBackLink();
    auto *fl = o.getFrontLink();
    {
        const float* p = o.getPosition();
        const float a = o.getRotation();
        const float w = o.getWidth();
        const float b = o.getBreadth();
        const float br = o.getBorder();
        const float h = o.getHeight();
        const float xtxcd = 1.0f;
        float ytxcd;
        if ((b - br) > 0.0f)
            ytxcd = h / (2.0f * (b - br));
        else
            ytxcd = 1.0f;
        const float cos_val = cosf(a);
        const float sin_val = sinf(a);
        const float params[4][2] =
        {{-1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};
        float wlen[2] = { (cos_val * w), (sin_val * w) };
        float blen[2] = { (-sin_val * (b - br)), (cos_val * (b - br)) };
        float verts[4][3];
        for (int i = 0; i < 4; ++i) {
            verts[i][0] = p[0] + (wlen[0] + (blen[0] * params[i][0]));
            verts[i][1] = p[1] + (wlen[1] + (blen[1] * params[i][0]));
            verts[i][2] = p[2] + ((h - br) * params[i][1]);
        }
        sEdge[0] = verts[1][0] - verts[0][0];
        sEdge[1] = verts[1][1] - verts[0][1];
        sEdge[2] = verts[1][2] - verts[0][2];
        tEdge[0] = verts[3][0] - verts[0][0];
        tEdge[1] = verts[3][1] - verts[0][1];
        tEdge[2] = verts[3][2] - verts[0][2];

        teleObj.addMatMesh("LinkMaterial",
            WorldPrimitiveGenerator::quad(verts[0], sEdge, tEdge, 0, 0, xtxcd, ytxcd));
        // Flip s edge around
        sEdge[0] = -(verts[1][0] - verts[0][0]);
        sEdge[1] = -(verts[1][1] - verts[0][1]);
        sEdge[2] = -(verts[1][2] - verts[0][2]);
        teleObj.addMatMesh("LinkMaterial",
            WorldPrimitiveGenerator::quad(verts[1], sEdge, tEdge, 0, 0, xtxcd, ytxcd));
    }
    worldObjects.emplace_back(std::move(teleObj));
}

void WorldMeshGenerator::addGround(float worldSize) {
    float base[3], sEdge[3], tEdge[3];
    base[0] = -worldSize;
    base[1] = -worldSize;
    base[2] = -0.1f;
    sEdge[0] = 2*worldSize;
    sEdge[1] = 0.0f;
    sEdge[2] = 0.0f;
    tEdge[0] = -0.1f;
    tEdge[1] = 2*worldSize;
    tEdge[2] = -0.1f;

    WorldObject groundObj;

    float uRepeat = 1.0f;
    float vRepeat = 1.0f;

    const auto *mat = MAGNUMMATERIALMGR.findMaterial("GroundMaterial");
    if (mat) {
        const auto &tname = mat->getTexture(0);
        auto tex = MagnumTextureManager::instance().getTexture(tname.c_str());
        if (tex.texture) {
            float repeat = 100.0;
            if (BZDB.isSet("groundHighResTexRepeat")) {
                repeat = 1.0f/BZDB.eval("groundHighResTexRepeat");
            }
            uRepeat = 2*repeat*(worldSize)/tex.width;
            vRepeat = 2*repeat*(worldSize)/tex.height;
        }
    }

    groundObj.addMatMesh("GroundMaterial",
        WorldPrimitiveGenerator::quad(base, sEdge, tEdge, 0, 0, uRepeat, vRepeat));
    worldObjects.emplace_back(std::move(groundObj));
}

static bool translucentMaterial(const MagnumBZMaterial* mat)
{
    // translucent texture?
    MagnumTextureManager &tm = MagnumTextureManager::instance();
    TextureData faceTexture = {NULL, 0, 0};
    if (mat->getTextureCount() > 0)
    {
        faceTexture = tm.getTexture(mat->getTexture(0).c_str());
        if (mat->getUseTextureAlpha(0)) return true;
    }

    // translucent color?
    bool translucentColor = false;
    const DynamicColor* dyncol = DYNCOLORMGR.getColor(mat->getDynamicColor());
    if (dyncol == NULL)
    {
        if (mat->getDiffuse()[3] != 1.0f)
            translucentColor = true;
    }
    else if (dyncol->canHaveAlpha())
        translucentColor = true;

    // is the color used?
    if (translucentColor)
    {
        if (((faceTexture.texture != NULL) && mat->getUseColorOnTexture(0)) ||
                (faceTexture.texture == NULL))
        {
            // modulate with the color if asked to, or
            // if the specified texture was not available
            return true;
        }
    }

    return false;
}

// This function is mashed together from various SceneNodes in the old code
// as well as from a few miscellaneous places. The idea is to take a mesh
// obstacle and simply result in mesh/material data in a WorldObject
// No fussing around with all sorts of intermediate objects and weird
// internal state.
void WorldMeshGenerator::addMesh(const MeshObstacle& o) {
    WorldObject meshObj;

    // HELPER FUNCTIONS

    // Check for downward facing faces that are below the ground
    auto groundClippedFace = [](const MeshFace* face) {
        const float* plane = face->getPlane();
        if (plane[2] < -0.9f)
        {
            // plane is facing downwards
            const Extents& exts = face->getExtents();
            if (exts.maxs[2] < 0.001)
            {
                // plane is on or below the ground, ditch it
                return true;
            }
        }
        return false;
    };
    // Predicate for qsort, just sorts based on the pointers
    // (this is kinda dumb, but it's how the original did it
    auto sortByMaterial = [](const void* a, const void *b) {
        const MeshFace* faceA = *((const MeshFace* const *)a);
        const MeshFace* faceB = *((const MeshFace* const *)b);
        const bool noClusterA = faceA->noClusters();
        const bool noClusterB = faceB->noClusters();

        if (noClusterA && !noClusterB)
            return -1;
        if (noClusterB && !noClusterA)
            return +1;

        if (faceA->getMaterial() > faceB->getMaterial())
            return +1;
        else
            return -1;
    };
    // Autogen texcoords for faces that don't have them
    // This is only used for non-DrawInfo meshes
    auto makeTexcoords = [](const float* plane,
        const std::vector<Vector3>& vertices,
        std::vector<Vector2>& texcoords)
    {

        auto x = vertices[1] - vertices[0];
        auto y = Math::cross({plane[0], plane[1], plane[2]}, x);

        float len = Math::dot(x, x);
        if (len > 0.0f)
        {
            len = 1.0f / sqrtf(len);
            x *= len;
        }
        else
            return false;

        len = Math::dot(y, y);
        if (len > 0.0f)
        {
            len = 1.0f / sqrtf(len);
            y *= len;
        }
        else
            return false;

        const float uvScale = 8.0f;

        texcoords[0] = {0.0f, 0.0f};

        // Remember that mesh is based on GL_TRIANGLE_FAN
        const int count = vertices.size();
        for (int i = 1; i < count; i++)
        {
            Vector3 delta = vertices[i]-vertices[0];
            texcoords[i] = {Math::dot(delta, x) / uvScale, Math::dot(delta, y) / uvScale};
        }

        return true;
    };

    struct MeshNode
    {
        bool isFace;
        std::vector<const MeshFace*> faces;
    };
    std::vector<MeshNode> nodes;

    const MeshDrawInfo* drawInfo = o.getDrawInfo();
    bool useDrawInfo = (drawInfo != NULL) && drawInfo->isValid();

    auto getTransformMatrix = [&]()
    {
        Matrix4 xformMatrix;
        const float *xformMatrixRaw;
        const MeshTransform::Tool* xformTool = drawInfo->getTransformTool();

        if (xformTool != NULL)
        {
            xformMatrixRaw = xformTool->getMatrix();
            // Transpose, because xformTool gives a backwards result
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    xformMatrix[i][j] = xformMatrixRaw[(4*j)+i];
        }
        return xformMatrix;
    };

    // Immediately-invoked lambda for maximum correspondance with old code
    // Corresponds with setupFacesAndFrags() in MeshSceneNodeGenerator
    // Just ported this over like this to avoid having to rewrite branching logic
    if (!useDrawInfo) { // Meshes without DrawInfo are handled below
        [&](){
            const int faceCount = o.getFaceCount();
            const bool noMeshClusters = BZDB.isTrue("noMeshClusters");
            if (o.noClusters() || noMeshClusters || !BZDBCache::zbuffer)
            {
                for (int i = 0; i < faceCount; i++)
                {
                    MeshNode mn;
                    mn.isFace = true;
                    mn.faces.push_back(o.getFace(i));
                    nodes.push_back(mn);
                }
                return; // bail out
            }
            // build up a list of faces and fragments
            const MeshFace** sortList = new const MeshFace*[faceCount];

            // clip ground faces, and then sort the face list by material
            int count = 0;
            for (int i = 0; i < faceCount; i++)
            {
                const MeshFace* face = o.getFace(i);
                if (!groundClippedFace(face))
                {
                    sortList[count] = face;
                    count++;
                }
            }
            // I think this sort is useless now, since we group by material later anyway
            qsort(sortList, count, sizeof(MeshFace*), sortByMaterial);

            // make the faces and fragments
            int first = 0;
            while (first < count)
            {
                const MeshFace* firstFace = sortList[first];
                const MagnumBZMaterial* firstMat = firstFace->getMaterial();

                // see if this face needs to be drawn individually
                if (firstFace->noClusters() ||
                        (translucentMaterial(firstMat) &&
                        !firstMat->getNoSorting() && !firstMat->getGroupAlpha()))
                {
                    MeshNode mn;
                    mn.isFace = true;
                    mn.faces.push_back(firstFace);
                    nodes.push_back(mn);
                    first++;
                    continue;
                }

                // collate similar materials
                int last = first + 1;
                while (last < count)
                {
                    const MeshFace* lastFace = sortList[last];
                    const MagnumBZMaterial* lastMat = lastFace->getMaterial();
                    if (lastMat != firstMat)
                        break;
                    last++;
                }

                // make a face for singles, and a fragment otherwise
                if ((last - first) == 1)
                {
                    MeshNode mn;
                    mn.isFace = true;
                    mn.faces.push_back(firstFace);
                    nodes.push_back(mn);
                }
                else
                {
                    MeshNode mn;
                    mn.isFace = false;
                    for (int i = first; i < last; i++)
                        mn.faces.push_back(sortList[i]);
                    nodes.push_back(mn);
                }

                first = last;
            }
        }(); // Please forgive me for this
    } else { // We are using drawinfo
        // ================ DrawInfo Meshes handled below ================
        // These meshes were handled in an extremely convoluted way in the old code
        // As far as I can tell, meshes were parsed into lists to be sent to OpenGL
        // But the code was pretty fragile and pretty unencapsulated... The code
        // below is built up from sources such as MeshSceneNode.cxx, MeshSceneNodeGenerator.cxx
        // MeshDrawInfo.cxx, MeshRenderNode.cxx, among others.
        if (drawInfo->isInvisible()) {
            return;
        }

        const MeshTransform::Tool* xformTool = drawInfo->getTransformTool();

        // Get LOD (just get the highest level one for now, can expand this later)
        if (drawInfo->getLodCount() < 1) return;

        auto lods = drawInfo->getDrawLods();
        const DrawLod& lod = lods[0];   // Just get highest LOD for now

        // Grab transform matrix (transforms obj coords to world coords)
        Matrix4 transformMat = getTransformMatrix();

        // Copy out verts, norms, texcoords
        auto di_verts = drawInfo->getVertices();
        auto di_norms = drawInfo->getNormals();
        auto di_texcoords = drawInfo->getTexcoords();

        auto corners = drawInfo->getCorners();

        size_t cornerCount = drawInfo->getCornerCount();

        // For each set (pairing of command (GL_TRIANGLES, GL_POINTS, etc, and index data)

        for (int setnum = 0; setnum < lod.count; ++setnum) {
            const DrawSet& dset = lod.sets[setnum];
            // For each draw command in the draw set...
            for (int cmdnum = 0; cmdnum < dset.count; ++cmdnum) {
                const DrawCmd& cmd = dset.cmds[cmdnum];

                UnsignedInt rawIndexCount = cmd.count;

                // Draw commands store indices with runtime packing, unpack to Uints first
                std::vector<UnsignedInt> unpackedRawIndices;
                for (int i = 0; i < rawIndexCount; ++i) {
                    switch(cmd.indexType) {
                        case DrawCmd::DrawIndexUShort:
                            unpackedRawIndices.push_back((UnsignedInt)((unsigned short *)cmd.indices)[i]);
                            break;
                        case DrawCmd::DrawIndexUInt:
                            unpackedRawIndices.push_back((UnsignedInt)((unsigned int *)cmd.indices)[i]);
                            break;
                        default:
                            Warning{} << "Unsupported index format" << cmd.indexType;
                    }
                }

                // Materials referenced by DrawSet need to be fixed up, they
                // may be invalid without fixup, and will result in missing/default textures
                const MagnumBZMaterial *maybeFixedupMaterial = dset.material;
                const MagnumMaterialMap *matMap = drawInfo->getMaterialMap();
                if (matMap != NULL) {
                    MagnumMaterialMap::const_iterator it = matMap->find(dset.material);
                    if (it != matMap->end()) {
                        maybeFixedupMaterial = it->second;
                    }
                }

                // This will be used to index into our verts, texcoords, and normals
                // We will reorder these indices based on the DrawMode to produce
                // triangles, then emit a new set of arrays with indices 0..N
                std::vector<UnsignedInt> indices{};

                // Fix up indices
                switch (cmd.drawMode) {
                    case DrawCmd::DrawTriangles: {
                        // Just use the raw indices directly
                        indices = unpackedRawIndices;
                        break;
                    }

                    // Draw polys with triangle fan (polys must be convex)
                    case DrawCmd::DrawTriangleFan:
                    case DrawCmd::DrawPolygon: {
                        // Verts come in as a triangle fan, fixup the indices for triangles instead
                        // There are #indices-2 triangles
                        for (UnsignedInt i = 0; i < unpackedRawIndices.size()-2; ++i) {
                            indices.push_back(unpackedRawIndices[0]);
                            indices.push_back(unpackedRawIndices[i+1]);
                            indices.push_back(unpackedRawIndices[i+2]);
                        }
                        break;
                    }

                    case DrawCmd::DrawTriangleStrip:
                    case DrawCmd::DrawQuadStrip: {
                        // Verts come in as a triangle strip, fixup the indices for triangles instead
                        // There are #indices-2 triangles
                        for (UnsignedInt i = 0; i < unpackedRawIndices.size()-2; ++i) {
                            if (i%2) { // Odd case
                                indices.push_back(unpackedRawIndices[i+1]);
                                indices.push_back(unpackedRawIndices[i]);
                                indices.push_back(unpackedRawIndices[i+2]);
                            } else { // Even case
                                indices.push_back(unpackedRawIndices[i]);
                                indices.push_back(unpackedRawIndices[i+1]);
                                indices.push_back(unpackedRawIndices[i+2]);
                            }
                            
                        }
                        break;
                    }

                    case DrawCmd::DrawQuads: {
                        // Number of triangles is #indices/2
                        // Number of quads is #indices/4
                        for (UnsignedInt i = 0; i < unpackedRawIndices.size()/4; ++i) {
                            // Add 1 quad = 2 tris at a time
                            indices.push_back(unpackedRawIndices[4*i]);
                            indices.push_back(unpackedRawIndices[4*i+1]);
                            indices.push_back(unpackedRawIndices[4*i+2]);
                            indices.push_back(unpackedRawIndices[4*i+2]);
                            indices.push_back(unpackedRawIndices[4*i+3]);
                            indices.push_back(unpackedRawIndices[4*i]);
                        }
                        break;
                    }

                    case DrawCmd::DrawPoints:
                    case DrawCmd::DrawLines:
                    case DrawCmd::DrawLineLoop:
                    case DrawCmd::DrawLineStrip:
                    default:
                        Warning{} << "Unsupported DrawInfo draw command" << cmd.drawMode;
                }
                if (indices.size() > 0) {
                    std::vector<Math::Vector3<float>> verts{indices.size()};
                    for (int i = 0; i < indices.size(); ++i) {
                        verts[i].x() = di_verts[indices[i]][0];
                        verts[i].y() = di_verts[indices[i]][1];
                        verts[i].z() = di_verts[indices[i]][2];
                        verts[i] = transformMat.transformPoint(verts[i]);
                    }

                    std::vector<Math::Vector3<float>> norms{indices.size()};
                    for (int i = 0; i < indices.size(); ++i) {
                        norms[i].x() = di_norms[indices[i]][0];
                        norms[i].y() = di_norms[indices[i]][1];
                        norms[i].z() = di_norms[indices[i]][2];
                        norms[i] = transformMat.transformVector(norms[i]);
                    }

                    std::vector<Math::Vector2<float>> texcoords{indices.size()};
                    for (int i = 0; i < indices.size(); ++i) {
                        texcoords[i].x() = di_texcoords[indices[i]][0];
                        texcoords[i].y() = di_texcoords[indices[i]][1];
                    }
                    std::vector<UnsignedInt> linearizedIndices(indices.size());
                    for (int i = 0; i < indices.size(); ++i) {
                        linearizedIndices[i] = i;
                    }
                    meshObj.addMatMesh(maybeFixedupMaterial->getName(),
                        WorldPrimitiveGenerator::rawIndexedTris(verts, norms, texcoords, linearizedIndices));
                }
            }
        }
        worldObjects.emplace_back(std::move(meshObj));
        return;
    }

    // ==================== We have a non-DrawInfo Mesh =======================
    // These are pretty straightforward, grab the faces, do any necessary post-processing
    // (pack verts, normals, and texcoords) and ship them off to WorldPrimitiveGenerator
    // to be processed into MeshData. Non-DrawInfo mesh faces are always indexed meshes
    // in the form of triangle fans, so we post-process the indices to convert them
    // into triangles instead, for uniformity with other mesh handling code.

    //while ((node = nodeGen->getNextNode(wallLOD)))
    int currentNode = 0;
    // Corresponds to MeshSceneNodeGenerator::getNextNode()
    auto genNextPoly = [&]() {
        const MeshNode* mn;
        const MeshFace* face;
        const MagnumBZMaterial* mat;

        /*if (useDrawInfo) {
            if (drawInfo->isInvisible())
            {
                if (occluders.size() <= 0)
                    return NULL;
                else
                {
                    currentNode = 1;
                    returnOccluders = true;
                    return (WallSceneNode*)occluders[0];
                }
            }
            else
            {
                currentNode = 0;
                returnOccluders = true;
                return (WallSceneNode*)(new MeshSceneNode(mesh));
            }
        }*/
        // remove any faces or frags that will not be displayed
        // also, return NULL if we are at the end of the face list
        while (true)
        {

            if (currentNode >= (int)nodes.size())
            {
                // start sending out the occluders
                /*returnOccluders = true;
                if (occluders.size() > 0)
                {
                    currentNode = 1;
                    return (WallSceneNode*)occluders[0];
                }
                else
                    return NULL;*/
                return false;
            }

            mn = &nodes[currentNode];
            if (mn->isFace)
            {
                face = mn->faces[0];
                mat = face->getMaterial();
            }
            else
            {
                face = NULL;
                mat = mn->faces[0]->getMaterial();
            }

            if (mat->isInvisible())
            {
                currentNode++;
                continue;
            }

            if (mn->isFace && groundClippedFace(face))
            {
                currentNode++;
                continue;
            }

            break; // break the loop if we haven't used 'continue'
        }

        if (mn->isFace) {
            const size_t vertexCount = face->getVertexCount();
            std::vector<Math::Vector3<float>> verts{vertexCount};
            for (int i = 0; i < vertexCount; ++i) {
                const float * vp = face->getVertex(i);
                verts[i].x() = vp[0];
                verts[i].y() = vp[1];
                verts[i].z() = vp[2];
            }

            std::vector<Math::Vector3<float>> norms{vertexCount};
            for (int i = 0; i < vertexCount; ++i) {
                const float * vp;
                if (face->useNormals()) vp = face->getNormal(i);
                else vp = face->getPlane();
                norms[i].x() = vp[0];
                norms[i].y() = vp[1];
                norms[i].z() = vp[2];
            }

            std::vector<Math::Vector2<float>> texcoords{vertexCount};
            if (face->useTexcoords()) {
                for (int i = 0; i < vertexCount; ++i) {
                    const float * vp = face->getTexcoord(i);
                    texcoords[i].x() = vp[0];
                    texcoords[i].y() = vp[1];
                }
            } else {
                makeTexcoords(face->getPlane(), verts, texcoords);
                
            }
            meshObj.addMatMesh(mat->getName(),
            WorldPrimitiveGenerator::planarPolyFromTriFan(verts, norms, texcoords));
    
        }
        else
        {
            // The old mesh code stored a bunch of faces on one mesh node
            // in the case that it wanted to compile them into buffers
            // to be rendered as triangles... that's what we're doing
            // for ALL the faces anyway, so we can just pack each
            // face separately into the meshObj.
            int faceCount = mn->faces.size();

            for (int i = 0; i < faceCount; ++i) {
                const MeshFace* face = mn->faces[i];
                const size_t vertexCount = face->getVertexCount();

                std::vector<Math::Vector3<float>> verts{vertexCount};
                for (int i = 0; i < vertexCount; ++i) {
                    const float * vp = face->getVertex(i);
                    verts[i].x() = vp[0];
                    verts[i].y() = vp[1];
                    verts[i].z() = vp[2];
                }

                std::vector<Math::Vector2<float>> texcoords{vertexCount};
                if (face->useTexcoords()) {
                    for (int i = 0; i < vertexCount; ++i) {
                        const float * vp = face->getTexcoord(i);
                        texcoords[i].x() = vp[0];
                        texcoords[i].y() = vp[1];
                    }
                } else {
                    makeTexcoords(face->getPlane(), verts, texcoords);
                }

                std::vector<Math::Vector3<float>> norms{vertexCount};
                for (int i = 0; i < vertexCount; ++i) {
                    const float * vp;
                    if (face->useNormals()) vp = face->getNormal(i);
                    else vp = face->getPlane();
                    norms[i].x() = vp[0];
                    norms[i].y() = vp[1];
                    norms[i].z() = vp[2];
                }

                meshObj.addMatMesh(mat->getName(),
                WorldPrimitiveGenerator::planarPolyFromTriFan(verts, norms, texcoords));


            }
        }

        currentNode++;

        return true;

    };

    while (genNextPoly()) {}
    worldObjects.emplace_back(std::move(meshObj));

}

std::vector<std::string> WorldMeshGenerator::getMaterialList() const {
    std::set<std::string> mats;
    std::vector<std::string> matnames;
    for (const auto& e: worldObjects) {
        for (const auto& mm: e.getMatMeshes()) {
            mats.insert(mm.first);
        }
    }
    for (const auto& e: mats) {
        matnames.emplace_back(e);
    }
    return matnames;
}

void WorldMeshGenerator::reset() {
    worldObjects.clear();
}

GL::Mesh WorldMeshGenerator::compileMatMesh(std::string matname) const {

    Containers::Array<Vector3> positions;
    Containers::Array<Vector3> normals;
    Containers::Array<Vector2> texcoords;
    Containers::Array<UnsignedInt> indices;
    int vertexCount = 0;
    int indOffset = 0;
    for (const auto& o: worldObjects) {
        for (const auto& mm: o.getMatMeshes()) {
            if (mm.first == matname) {
                const auto& md = mm.second;
                Containers::arrayAppend(positions, Containers::ArrayView<const Vector3>{md.getVertices().data(), md.getVertices().size()});
                Containers::arrayAppend(normals, Containers::ArrayView<const Vector3>{md.getNormals().data(), md.getNormals().size()});
                Containers::arrayAppend(texcoords, Containers::ArrayView<const Vector2>{md.getTexcoords().data(), md.getTexcoords().size()});
                Containers::Array<UnsignedInt> inds{md.getIndices().size()};

                for (int i = 0; i < inds.size(); ++i) {
                    inds[i] = md.getIndices()[i] + indOffset;
                }

                vertexCount += md.getVertices().size();
                indOffset += md.getVertices().size();
                Containers::arrayAppend(indices, inds);
            }
        }
    }

    Containers::Array<char> meshdata = MeshTools::interleave(positions, normals, texcoords);
    Containers::Pair<Containers::Array<char>, MeshIndexType> idxraw = MeshTools::compressIndices(indices);

    GL::Mesh ret;;
    GL::Buffer indicesbuf{GL::Buffer::TargetHint::ElementArray};
    indicesbuf.setData(idxraw.first());
    GL::Buffer vbuf{GL::Buffer::TargetHint::Array};
    vbuf.setData(meshdata);

    ret.addVertexBuffer(std::move(vbuf), 0, Shaders::PhongGL::Position{}, Shaders::PhongGL::Normal{}, Shaders::PhongGL::TextureCoordinates{});
    ret.setIndexBuffer(std::move(indicesbuf), 0, idxraw.second());
    ret.setCount(indices.size());
    return ret;
    
}
