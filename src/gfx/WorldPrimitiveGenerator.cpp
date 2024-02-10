#include "IndexedMeshData.h"
#include "Magnum/Types.h"
#include "MagnumDefs.h"
#include "WorldPrimitiveGenerator.h"

#include <Magnum/MeshTools/Duplicate.h>
#include <Magnum/MeshTools/GenerateFlatNormals.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/Copy.h>

using namespace Magnum;
using namespace Magnum::Math::Literals;

IndexedMeshData WorldPrimitiveGenerator::quad(const float base[3], const float uEdge[3], const float vEdge[3], float uOffset, float vOffset, float uRepeats, float vRepeats) {
    const std::vector<Vector3> vertices{
        {{base[0] + uEdge[0], base[1] + uEdge[1], base[2] + uEdge[2]}}, /* Bottom right */
        {{base[0] + uEdge[0] + vEdge[0], base[1] + uEdge[1] + vEdge[1], base[2] + uEdge[2] + vEdge[2]}}, /* Top right */
        {{base[0], base[1], base[2]}}, /* Bottom left */
        {{base[0] + vEdge[0], base[1] + vEdge[1], base[2] + vEdge[2]}}  /* Top left */
    };
    const std::vector<Vector2> texcoords{
        {uOffset + uRepeats, 0.0f},
        {uOffset + uRepeats, vOffset + vRepeats},
        {uOffset, vOffset},
        {uOffset, vOffset + vRepeats}
    };
    // Compute normal (same for all verts)
    float plane[4];
    plane[0] = uEdge[1] * vEdge[2] - uEdge[2] * vEdge[1];
    plane[1] = uEdge[2] * vEdge[0] - uEdge[0] * vEdge[2];
    plane[2] = uEdge[0] * vEdge[1] - uEdge[1] * vEdge[0];
    plane[3] = -(plane[0] * base[0] + plane[1] * base[1]
                + plane[2] * base[2]);
    // get normalization factor
    const float n = 1.0f / sqrtf((plane[0] * plane[0]) +
                                 (plane[1] * plane[1]) +
                                 (plane[2] * plane[2]));

    // store normalized plane equation
    plane[0] = n * plane[0];
    plane[1] = n * plane[1];
    plane[2] = n * plane[2];
    plane[3] = n * plane[3];
    const std::vector<Vector3> normals{
        {plane[0], plane[1], plane[2]},
        {plane[0], plane[1], plane[2]},
        {plane[0], plane[1], plane[2]},
        {plane[0], plane[1], plane[2]}
    };
    const std::vector<UnsignedInt> indices{        /* 3--1 1 */
        0, 1, 2,                        /* | / /| */
        2, 1, 3                         /* |/ / | */
    };

    return IndexedMeshData(std::move(vertices), std::move(normals), std::move(texcoords), std::move(indices));
}

IndexedMeshData WorldPrimitiveGenerator::tri(const float base[3], const float uEdge[3], const float vEdge[3], float uOffset, float vOffset, float uRepeats, float vRepeats) {
    const std::vector<Vector3> vertices{
        {{base[0], base[1], base[2]}},
        {{base[0] + uEdge[0], base[1] + uEdge[1], base[2] + uEdge[2]}},
        {{base[0] + vEdge[0], base[1] + vEdge[1], base[2] + vEdge[2]}}
    };
    const std::vector<Vector2> texcoords{
        {uOffset, vOffset},
        {uOffset + uRepeats, vOffset},
        {uOffset, vOffset + vRepeats},
    };
        // Compute normal (same for all verts)
    float plane[4];
    plane[0] = uEdge[1] * vEdge[2] - uEdge[2] * vEdge[1];
    plane[1] = uEdge[2] * vEdge[0] - uEdge[0] * vEdge[2];
    plane[2] = uEdge[0] * vEdge[1] - uEdge[1] * vEdge[0];
    plane[3] = -(plane[0] * base[0] + plane[1] * base[1]
                + plane[2] * base[2]);
    // get normalization factor
    const float n = 1.0f / sqrtf((plane[0] * plane[0]) +
                                 (plane[1] * plane[1]) +
                                 (plane[2] * plane[2]));

    // store normalized plane equation
    plane[0] = n * plane[0];
    plane[1] = n * plane[1];
    plane[2] = n * plane[2];
    plane[3] = n * plane[3];
    const std::vector<Vector3> normals{
        {plane[0], plane[1], plane[2]},
        {plane[0], plane[1], plane[2]},
        {plane[0], plane[1], plane[2]}
    };
    const std::vector<UnsignedInt> indices{
        0, 1, 2,
    };

    return IndexedMeshData(std::move(vertices), std::move(normals), std::move(texcoords), std::move(indices));
}

IndexedMeshData WorldPrimitiveGenerator::planarPolyFromTriFan(const std::vector<Magnum::Math::Vector3<float>> &verts, const std::vector<Magnum::Math::Vector3<float>> &norms, const std::vector<Magnum::Math::Vector2<float>> &texcoords) {
    // Num triangles = 3(N-2) for triangle fan
    std::vector<UnsignedInt> indices(3*(verts.size()-2));

    // Verts come in as a triangle fan, fixup the indices for triangles instead
    // There are verts-2 triangles
    for (UnsignedInt i = 0; i < verts.size()-2; ++i) {
        indices[i*3] = 0;
        indices[i*3+1] = i+1;
        indices[i*3+2] = i+2;
    }

    return IndexedMeshData(std::move(verts), std::move(norms), std::move(texcoords), std::move(indices));
}

IndexedMeshData WorldPrimitiveGenerator::rawIndexedTris(const std::vector<Magnum::Math::Vector3<float>> &verts, const std::vector<Magnum::Math::Vector3<float>> &norms, const std::vector<Magnum::Math::Vector2<float>> &texcoords, const std::vector<UnsignedInt> &indices) {
    return IndexedMeshData(std::move(verts), std::move(norms), std::move(texcoords), std::move(indices));
}

Trade::MeshData WorldPrimitiveGenerator::debugLine(Magnum::Math::Vector3<float> a, Magnum::Math::Vector3<float> b) {
    constexpr Trade::MeshAttributeData AttributeData3DWireframe[]{
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, VertexFormat::Vector3,
            0, 0, sizeof(Vector3)}
    };
    constexpr UnsignedInt points = 11;

    Containers::Array<char> vertexData{points*sizeof(Vector3)};
    auto positions = Containers::arrayCast<Vector3>(vertexData);
    Vector3 delta = b-a;
    delta /= float(points)-1;

    Vector3 pos = a;
    for (int i = 0; i < points; ++i) {
        positions[i] = pos+delta*i;
    }

    return MeshTools::copy(Trade::MeshData{MeshPrimitive::LineStrip, Utility::move(vertexData),
        Trade::meshAttributeDataNonOwningArray(AttributeData3DWireframe), UnsignedInt(positions.size())});
}
