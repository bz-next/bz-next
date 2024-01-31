#include "Magnum/Types.h"
#include "MagnumDefs.h"
#include "WorldPrimitiveGenerator.h"

#include <Magnum/MeshTools/Duplicate.h>
#include <Magnum/MeshTools/GenerateFlatNormals.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/Copy.h>

using namespace Magnum;
using namespace Magnum::Math::Literals;

Trade::MeshData WorldPrimitiveGenerator::quad(const float base[3], const float uEdge[3], const float vEdge[3], float uOffset, float vOffset, float uRepeats, float vRepeats) {
    const Vector3 vertices[]{
        {{base[0] + uEdge[0], base[1] + uEdge[1], base[2] + uEdge[2]}}, /* Bottom right */
        {{base[0] + uEdge[0] + vEdge[0], base[1] + uEdge[1] + vEdge[1], base[2] + uEdge[2] + vEdge[2]}}, /* Top right */
        {{base[0], base[1], base[2]}}, /* Bottom left */
        {{base[0] + vEdge[0], base[1] + vEdge[1], base[2] + vEdge[2]}}  /* Top left */
    };
    const Vector2 texcoords[]{
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
    const Vector3 normals[]{
        {plane[0], plane[1], plane[2]},
        {plane[0], plane[1], plane[2]},
        {plane[0], plane[1], plane[2]},
        {plane[0], plane[1], plane[2]}
    };
    const UnsignedInt indices[]{        /* 3--1 1 */
        0, 1, 2,                        /* | / /| */
        2, 1, 3                         /* |/ / | */
    };

    struct VertexData {
        Vector3 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    Containers::ArrayView<const Vector3> posview{vertices};
    Containers::ArrayView<const Vector2> texview{texcoords};
    Containers::ArrayView<const Vector3> normview{normals};

    // Pack mesh data
    Containers::Array<char> data{posview.size()*sizeof(VertexData)};
    data = MeshTools::interleave(posview, texview, normview);
    Containers::StridedArrayView1D<const VertexData> dataview = Containers::arrayCast<const VertexData>(data);

    return MeshTools::copy(Trade::MeshData {MeshPrimitive::Triangles, Trade::DataFlags{}, indices, Trade::MeshIndexData{indices}, std::move(data), {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, dataview.slice(&VertexData::position)},
        Trade::MeshAttributeData{Trade::MeshAttribute::TextureCoordinates, dataview.slice(&VertexData::texcoord)},
        Trade::MeshAttributeData{Trade::MeshAttribute::Normal, dataview.slice(&VertexData::normal)}
    }, static_cast<UnsignedInt>(dataview.size())});

}

Trade::MeshData WorldPrimitiveGenerator::tri(const float base[3], const float uEdge[3], const float vEdge[3], float uOffset, float vOffset, float uRepeats, float vRepeats) {
    const Vector3 vertices[]{
        {{base[0], base[1], base[2]}},
        {{base[0] + uEdge[0], base[1] + uEdge[1], base[2] + uEdge[2]}},
        {{base[0] + vEdge[0], base[1] + vEdge[1], base[2] + vEdge[2]}}
    };
    const Vector2 texcoords[]{
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
    const Vector3 normals[]{
        {plane[0], plane[1], plane[2]},
        {plane[0], plane[1], plane[2]},
        {plane[0], plane[1], plane[2]}
    };
    const UnsignedInt indices[]{
        0, 1, 2,
    };

    struct VertexData {
        Vector3 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    Containers::ArrayView<const Vector3> posview{vertices};
    Containers::ArrayView<const Vector2> texview{texcoords};
    Containers::ArrayView<const Vector3> normview{normals};

    // Pack mesh data
    Containers::Array<char> data{posview.size()*sizeof(VertexData)};
    data = MeshTools::interleave(posview, texview, normview);
    Containers::StridedArrayView1D<const VertexData> dataview = Containers::arrayCast<const VertexData>(data);

    return MeshTools::copy(Trade::MeshData {MeshPrimitive::Triangles, Trade::DataFlags{}, indices, Trade::MeshIndexData{indices}, std::move(data), {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, dataview.slice(&VertexData::position)},
        Trade::MeshAttributeData{Trade::MeshAttribute::TextureCoordinates, dataview.slice(&VertexData::texcoord)},
        Trade::MeshAttributeData{Trade::MeshAttribute::Normal, dataview.slice(&VertexData::normal)}
    }, static_cast<UnsignedInt>(dataview.size())});
}

Magnum::Trade::MeshData WorldPrimitiveGenerator::planarPoly(const std::vector<Magnum::Math::Vector3<float>> &verts, const std::vector<Magnum::Math::Vector3<float>> &norms, const std::vector<Magnum::Math::Vector2<float>> &texcoords) {

    struct VertexData {
        Vector3 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    Containers::ArrayView<const Vector3> posview{verts};
    Containers::ArrayView<const Vector2> texview{texcoords};
    Containers::ArrayView<const Vector3> normview{norms};

    // Num triangles = 3(N-2) for triangle fan
    Containers::Array<UnsignedInt> indices{3*(verts.size()-2)};

    // Verts come in as a triangle fan, fixup the indices for triangles instead
    // There are verts-2 triangles
    for (UnsignedInt i = 0; i < verts.size()-2; ++i) {
        indices[i*3] = 0;
        indices[i*3+1] = i+1;
        indices[i*3+2] = i+2;
    }

    // Pack mesh data
    Containers::Array<char> data{posview.size()*sizeof(VertexData)};
    data = MeshTools::interleave(posview, texview, normview);
    Containers::StridedArrayView1D<const VertexData> dataview = Containers::arrayCast<const VertexData>(data);

    return MeshTools::copy(Trade::MeshData {MeshPrimitive::Triangles, Trade::DataFlags{}, indices, Trade::MeshIndexData{indices}, std::move(data), {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, dataview.slice(&VertexData::position)},
        Trade::MeshAttributeData{Trade::MeshAttribute::TextureCoordinates, dataview.slice(&VertexData::texcoord)},
        Trade::MeshAttributeData{Trade::MeshAttribute::Normal, dataview.slice(&VertexData::normal)}
    }, static_cast<UnsignedInt>(dataview.size())});
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
