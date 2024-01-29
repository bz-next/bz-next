#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Magnum/MeshTools/Interleave.h>

#include "MagnumTextureManager.h"
#include "WorldPrimitiveGenerator.h"
#include "Magnum/Trade/MeshData.h"
#include "MagnumBZMaterial.h"

#include "StateDatabase.h"
#include "WorldSceneBuilder.h"
#include "BoxBuilding.h"

#include <utility>

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
    // The old code mapped textures straight to the pyr without a material
    // Instead, we assume we have a material called pyrWallMaterial
    const MagnumBZMaterial *pyrWallMat = MAGNUMMATERIALMGR.findMaterial("pyrWallMaterial");
    float texFactor = BZDB.eval("pyrWallTexRepeat");

    // The original pyramid code uses the box texture size as a magic texture scaling parameter...
    // needless to say, this should be changed
    auto * bwtex = MagnumTextureManager::instance().getTexture("boxwall");
    float boxTexWidth, boxTexHeight;
    boxTexWidth = boxTexHeight = 0.2f * BZDB.eval(StateDatabase::BZDB_BOXHEIGHT);
    if (bwtex)
        boxTexWidth = (float)bwtex->imageSize(0)[0] / (float)bwtex->imageSize(0)[1] * boxTexHeight;

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
    return Trade::MeshData {MeshPrimitive::Triangles, Trade::DataFlags{}, std::move(indices), Trade::MeshIndexData{indices}, std::move(meshdata), {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, dataview.slice(&VertexData::position)},
        Trade::MeshAttributeData{Trade::MeshAttribute::TextureCoordinates, dataview.slice(&VertexData::texcoord)},
        Trade::MeshAttributeData{Trade::MeshAttribute::Normal, dataview.slice(&VertexData::normal)}
        }, static_cast<UnsignedInt>(vertexCount)};
}