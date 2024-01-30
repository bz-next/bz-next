#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/Copy.h>


#include "Magnum/GL/GL.h"
#include "MagnumTextureManager.h"
#include "WorldPrimitiveGenerator.h"
#include "Magnum/Trade/MeshData.h"
#include "MagnumBZMaterial.h"

#include "StateDatabase.h"
#include "WorldSceneBuilder.h"
#include "BoxBuilding.h"
#include "BaseBuilding.h"
#include "Team.h"

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
        if (faceNum <= 4) {
            uRepeats = -texFactor*boxTexWidth;
            vRepeats = -texFactor*boxTexWidth;
        } else {
            uRepeats = -boxTexHeight;
            vRepeats = -boxTexHeight;
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

        uRepeats = -texFactor*boxTexHeight;
        vRepeats = -texFactor*boxTexHeight;

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
        if (faceNum >= 1 && height == 0) return false;
        if (faceNum >= 6) return false;
        if (height == 0) {
            bb.getCorner(0, base);
            bb.getCorner(3, tCorner);
            bb.getCorner(1, sCorner);
        }
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
