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
