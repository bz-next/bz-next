
#include "MagnumTextureManager.h"
#include "WorldPrimitiveGenerator.h"
#include "Magnum/Trade/MeshData.h"
#include "MagnumBZMaterial.h"

#include "StateDatabase.h"
#include "WorldSceneBuilder.h"
#include "BoxBuilding.h"

#include <utility>

using namespace Magnum;

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
    float texFactor = BZDB.eval("boxWallTexRepeat");

    WorldObject boxObj;

    // Annoyingly, boxes don't use a TextureMatrix to scale the texture...
    // instead the UV coordinates are fudged. We will do this for now,
    // but in the future the box material should just be updated with a
    // TextureMatrix
    // TODO: Add a texmat to box materials and remove this!
    auto * bwtex = MagnumTextureManager::instance().getTexture("boxwall");
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
            }
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
    };

    for (int i = 1; i <= 6; ++i) {
        computeEdges(o, i);
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
