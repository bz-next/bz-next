#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/Copy.h>
#include <Magnum/Math/Vector.h>


#include "Magnum/GL/GL.h"
#include "Magnum/Trade/MeshData.h"


#include "MagnumTextureManager.h"
#include "MeshObstacle.h"
#include "WorldPrimitiveGenerator.h"
#include "MagnumBZMaterial.h"

#include "StateDatabase.h"
#include "WorldSceneBuilder.h"
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

void WorldObject::addMatMesh(std::string materialname, Trade::MeshData&& md) {
    matMeshes.emplace_back(std::make_pair(materialname, std::move(md)));
}

const std::vector<MatMesh>& WorldObject::getMatMeshes() const {
    return matMeshes;
}

void WorldSceneBuilder::addBox(BoxBuilding& o) {
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

    Magnum::GL::Texture2D *bwtex = NULL;// = MagnumTextureManager::instance().getTexture("boxwall");

    if (o.userTextures[0].size())
        bwtex = tm.getTexture(o.userTextures[0].c_str());
    if (bwtex == NULL)
        bwtex = MagnumTextureManager::instance().getTexture(BZDB.get("boxWallTexture").c_str());

    float boxTexWidth, boxTexHeight;
    boxTexWidth = boxTexHeight = 0.2f * BZDB.eval(StateDatabase::BZDB_BOXHEIGHT);
    if (bwtex)
        boxTexWidth = (float)bwtex->imageSize(0)[0] / (float)bwtex->imageSize(0)[1] * boxTexHeight;


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

void WorldSceneBuilder::addPyr(PyramidBuilding& o) {
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

void WorldSceneBuilder::addBase(BaseBuilding& o) {
    // The old code mapped textures straight to the box without a material
    // Instead, we assume we have materials called boxWallMaterial and
    // boxTopMaterial loaded earlier when initializing the program
    const MagnumBZMaterial *boxWallMat = MAGNUMMATERIALMGR.findMaterial(Team::getImagePrefix((TeamColor)o.getTeam()) + "boxWallMaterial");
    const MagnumBZMaterial *boxTopMat = MAGNUMMATERIALMGR.findMaterial(Team::getImagePrefix((TeamColor)o.getTeam()) + "boxTopMaterial");

    auto &tm = MagnumTextureManager::instance();

    float texFactor = BZDB.eval("boxWallTexRepeat");

    WorldObject baseObj;

    Magnum::GL::Texture2D *bwtex = NULL;

    if (o.userTextures[0].size())
        bwtex = tm.getTexture(o.userTextures[0].c_str());
    if (bwtex == NULL) {
        std::string teamBase = Team::getImagePrefix((TeamColor)o.getTeam());
        teamBase += BZDB.get("baseWallTexture");
        bwtex = tm.getTexture(teamBase.c_str());
    }
    if (bwtex == NULL)
        bwtex = tm.getTexture(BZDB.get("boxWallTexture").c_str());

    Magnum::GL::Texture2D *bttex = NULL;

    if (o.userTextures[1].size())
        bttex = tm.getTexture(o.userTextures[1].c_str());
    if (bwtex == NULL) {
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

void WorldSceneBuilder::addWall(WallObstacle& o) {
    WorldObject wallObj;
    if (o.getHeight() <= 0.0f)
        return;

    const MagnumBZMaterial *wallMat = MAGNUMMATERIALMGR.findMaterial("wallMaterial");
    auto &tm = MagnumTextureManager::instance();


    // make styles -- first the outer wall
    auto *wallTexture = tm.getTexture( "wall" );
    float wallTexWidth, wallTexHeight;
    wallTexWidth = wallTexHeight = 10.0f;
    if (wallTexture != NULL)
        wallTexWidth = (float)wallTexture->imageSize(0)[0] / (float)wallTexture->imageSize(0)[1] * wallTexHeight;

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

void WorldSceneBuilder::addTeleporter(const Teleporter& o) {
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

void WorldSceneBuilder::addGround(float worldSize) {
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
    const auto *mat = MAGNUMMATERIALMGR.findMaterial("GroundMaterial");
    const auto &tname = mat->getTexture(0);
    auto* tex = MagnumTextureManager::instance().getTexture(tname.c_str());
    float uRepeat = 1.0f;
    float vRepeat = 1.0f;
    if (tex) {
        float repeat = 100.0;
        if (BZDB.isSet("groundHighResTexRepeat")) {
            repeat = 1.0f/BZDB.eval("groundHighResTexRepeat");
        }
        uRepeat = 2*repeat*(worldSize)/tex->imageSize(0)[0];
        vRepeat = 2*repeat*(worldSize)/tex->imageSize(0)[1];;
    }
    Warning{} << "World size " << worldSize;
    groundObj.addMatMesh("GroundMaterial",
        WorldPrimitiveGenerator::quad(base, sEdge, tEdge, 0, 0, uRepeat, vRepeat));
    worldObjects.emplace_back(std::move(groundObj));
}

static bool translucentMaterial(const MagnumBZMaterial* mat)
{
    // translucent texture?
    MagnumTextureManager &tm = MagnumTextureManager::instance();
    GL::Texture2D *faceTexture = NULL;
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
        if (((faceTexture != NULL) && mat->getUseColorOnTexture(0)) ||
                (faceTexture == NULL))
        {
            // modulate with the color if asked to, or
            // if the specified texture was not available
            return true;
        }
    }

    return false;
}


void WorldSceneBuilder::addMesh(const MeshObstacle& o) {
    WorldObject meshObj;
    // HELPER FUNCTIONS
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

    struct MeshNode
    {
        bool isFace;
        std::vector<const MeshFace*> faces;
    };
    std::vector<MeshNode> nodes;

    const MeshDrawInfo* drawInfo = o.getDrawInfo();
    bool useDrawInfo = (drawInfo != NULL) && drawInfo->isValid();

    // Immediately-invoked lambda for maximum correspondance with old code
    // Corresponds with setupFacesAndFrags() in MeshSceneNodeGenerator
    if (!useDrawInfo) [&](){
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
    }();

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
        //WallSceneNode* node;
        if (mn->isFace) {
            //node = getMeshPolySceneNode(face);
            // Corresponds to getMeshPolySceneNode(face)
            const size_t vertexCount = face->getVertexCount();
            std::vector<Math::Vector3<float>> verts{vertexCount};
            for (int i = 0; i < vertexCount; ++i) {
                const float * vp = face->getVertex(i);
                verts[i].x() = vp[0];
                verts[i].y() = vp[1];
                verts[i].z() = vp[2];
            }

            std::size_t normalCount = 0;
            if (face->useNormals())
                normalCount = vertexCount;
            //if (!face->useNormals()) {currentNode += 1; return true; } // Debug skip
            std::vector<Math::Vector3<float>> norms{vertexCount};
            for (int i = 0; i < normalCount; ++i) {
                const float * vp = face->getNormal(i);
                norms[i].x() = 0.0;//vp[0];
                norms[i].y() = 0.0;//vp[1];
                norms[i].z() = 1.0;//vp[2];
            }

            std::vector<Math::Vector2<float>> texcoords{vertexCount};
            if (face->useTexcoords()) {
                for (int i = 0; i < vertexCount; ++i) {
                    const float * vp = face->getTexcoord(i);
                    texcoords[i].x() = vp[0];
                    texcoords[i].y() = vp[1];
                }
            } else {
                //makeTexcoords(face->getPlane(), vertices, texcoords);
            }
            meshObj.addMatMesh(mat->getName(),
            WorldPrimitiveGenerator::planarPoly(verts, norms, texcoords));
    
        }
        else
        {
            /*const MeshFace** faces = new const MeshFace*[mn->faces.size()];
            for (int i = 0; i < (int)mn->faces.size(); i++)
                faces[i] = mn->faces[i];
            // the MeshFragSceneNode will delete the faces
            node = new MeshFragSceneNode(mn->faces.size(), faces);*/
        }

        currentNode++;

        return true;

    };

    while (genNextPoly()) {}
    worldObjects.emplace_back(std::move(meshObj));

}

std::vector<std::string> WorldSceneBuilder::getMaterialList() const {
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

void WorldSceneBuilder::reset() {
    worldObjects.clear();
}

Trade::MeshData WorldSceneBuilder::compileMatMesh(std::string matname) const {
    struct VertexData {
        Vector3 position;
        Vector2 texcoord;
        Vector3 normal;
    };
    Containers::Array<Vector3> positions;
    Containers::Array<Vector2> texcoords;
    Containers::Array<Vector3> normals;
    Containers::Array<UnsignedInt> indices;
    int vertexCount = 0;
    int indOffset = 0;
    for (const auto& o: worldObjects) {
        for (const auto& mm: o.getMatMeshes()) {
            if (mm.first == matname) {
                const auto& md = mm.second;
                Containers::arrayAppend(positions, md.positions3DAsArray());
                Containers::arrayAppend(texcoords, md.textureCoordinates2DAsArray());
                Containers::arrayAppend(normals, md.normalsAsArray());
                Containers::Array<UnsignedInt> inds{md.indicesAsArray()};

                for (auto &i: inds) {
                    i += indOffset;
                }
                vertexCount += md.vertexCount();
                indOffset += md.vertexCount();
                Containers::arrayAppend(indices, inds);
            }
        }
    }

    Containers::Array<char> meshdata = MeshTools::interleave(positions, texcoords, normals);
    Containers::StridedArrayView1D<const VertexData> dataview = Containers::arrayCast<const VertexData>(meshdata);
    return MeshTools::copy(Trade::MeshData {MeshPrimitive::Triangles, Trade::DataFlags{}, indices, Trade::MeshIndexData{indices}, std::move(meshdata), {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, dataview.slice(&VertexData::position)},
        Trade::MeshAttributeData{Trade::MeshAttribute::TextureCoordinates, dataview.slice(&VertexData::texcoord)},
        Trade::MeshAttributeData{Trade::MeshAttribute::Normal, dataview.slice(&VertexData::normal)}
        }, static_cast<UnsignedInt>(vertexCount)});
}
