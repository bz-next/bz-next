#include "MagnumDefs.h"
#include "WorldPrimitiveGenerator.h"

#include <Magnum/MeshTools/Duplicate.h>
#include <Magnum/MeshTools/GenerateFlatNormals.h>
#include <Magnum/MeshTools/Interleave.h>

using namespace Magnum;
using namespace Magnum::Math::Literals;

Trade::MeshData WorldPrimitiveGenerator::pyrSolid() {
    // Create pyr mesh
    Vector3 pyrVerts[5] =
    {
        {-1.0f, -1.0f, 0.0f},
        {+1.0f, -1.0f, 0.0f},
        {+1.0f, +1.0f, 0.0f},
        {-1.0f, +1.0f, 0.0f},
        { 0.0f,  0.0f, 1.0f}
    };

    Magnum::UnsignedShort pyrIndices[] =
    {
        // base
        2, 1, 0,
        0, 3, 2,
        0, 4, 3,
        3, 4, 2,
        2, 4, 1,
        4, 0, 1
    };

    struct Vertex {
        Vector3 position;
        Vector3 normal;
    };

    Containers::Array<Vector3> positions = MeshTools::duplicate<typeof pyrIndices[0], Vector3>(pyrIndices, pyrVerts);
    Containers::Array<Vector3> normals = MeshTools::generateFlatNormals(positions);

    Containers::Array<char> vertexData{positions.size() * sizeof(Vertex)};
    vertexData = MeshTools::interleave(positions, normals);

    Containers::StridedArrayView1D<const Vertex> vertices = Containers::arrayCast<const Vertex>(vertexData);

    Trade::MeshAttributeData pyrAttributes[] {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, vertices.slice(&Vertex::position)},
        Trade::MeshAttributeData{Trade::MeshAttribute::Normal, vertices.slice(&Vertex::normal)}
    };
    
    return Trade::MeshData{MeshPrimitive::Triangles, std::move(vertexData),
        {
            Trade::MeshAttributeData{Trade::MeshAttribute::Position, vertices.slice(&Vertex::position)},
            Trade::MeshAttributeData{Trade::MeshAttribute::Normal, vertices.slice(&Vertex::normal)}
        }, (Magnum::UnsignedInt)positions.size()};
}

Trade::MeshData WorldPrimitiveGenerator::wall() {
    // Create pyr mesh
    Vector3 pyrVerts[5] =
    {
        {-1.0f, -1.0f, 0.0f},
        {+1.0f, -1.0f, 0.0f},
        {+1.0f, +1.0f, 0.0f},
        {-1.0f, +1.0f, 0.0f},
        { 0.0f,  0.0f, 1.0f}
    };

    Magnum::UnsignedShort pyrIndices[] =
    {
        // base
        2, 1, 0,
        0, 3, 2,
        0, 4, 3,
        3, 4, 2,
        2, 4, 1,
        4, 0, 1
    };

    struct Vertex {
        Vector3 position;
        Vector3 normal;
    };

    Containers::Array<Vector3> positions = MeshTools::duplicate<typeof pyrIndices[0], Vector3>(pyrIndices, pyrVerts);
    Containers::Array<Vector3> normals = MeshTools::generateFlatNormals(positions);

    Containers::Array<char> vertexData{positions.size() * sizeof(Vertex)};
    vertexData = MeshTools::interleave(positions, normals);

    Containers::StridedArrayView1D<const Vertex> vertices = Containers::arrayCast<const Vertex>(vertexData);

    Trade::MeshAttributeData pyrAttributes[] {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, vertices.slice(&Vertex::position)},
        Trade::MeshAttributeData{Trade::MeshAttribute::Normal, vertices.slice(&Vertex::normal)}
    };
    
    return Trade::MeshData{MeshPrimitive::Triangles, std::move(vertexData),
        {
            Trade::MeshAttributeData{Trade::MeshAttribute::Position, vertices.slice(&Vertex::position)},
            Trade::MeshAttributeData{Trade::MeshAttribute::Normal, vertices.slice(&Vertex::normal)}
        }, (Magnum::UnsignedInt)positions.size()};
}

Trade::MeshData WorldPrimitiveGenerator::quad(const float base[3], const float uEdge[3], const float vEdge[3], float uOffset, float vOffset, float uRepeats, float vRepeats) {
    const Vector3 vertices[]{
        {{base[0] + uEdge[0], base[1] + uEdge[1], base[2] + uEdge[2]}}, /* Bottom right */
        {{base[0] + + uEdge[0] + vEdge[0], base[1] + uEdge[1] + vEdge[1], base[2] + uEdge[2] + vEdge[2]}}, /* Top right */
        {{base[0], base[1], base[2]}}, /* Bottom left */
        {{base[0] + uEdge[0], base[1] + uEdge[1], base[2] + uEdge[2]}}  /* Top left */
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

    return Trade::MeshData {MeshPrimitive::Triangles, Trade::DataFlags{}, indices, Trade::MeshIndexData{indices}, std::move(data), {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, dataview.slice(&VertexData::position)},
        Trade::MeshAttributeData{Trade::MeshAttribute::TextureCoordinates, dataview.slice(&VertexData::texcoord)},
        Trade::MeshAttributeData{Trade::MeshAttribute::Normal, dataview.slice(&VertexData::normal)}
    }, static_cast<UnsignedInt>(dataview.size())};

}