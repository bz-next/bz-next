#include "IndexedMeshData.h"

#include <Corrade/Utility/Assert.h>

using namespace Magnum;

IndexedMeshData::IndexedMeshData(
    const std::vector<Magnum::Math::Vector3<float>> &&verts,
    const std::vector<Magnum::Math::Vector3<float>> &&norms,
    const std::vector<Magnum::Math::Vector2<float>> &&texcoords,
    const std::vector<Magnum::UnsignedInt> &&indices) :
    meshVertices(std::move(verts)),
    meshNormals(std::move(norms)),
    meshTexcoords(std::move(texcoords)),
    meshIndices(std::move(indices)) {

    CORRADE_ASSERT(((verts.size() == norms.size()) && (verts.size() == texcoords.size())), "Malformed mesh data!", );
}

IndexedMeshData::IndexedMeshData(
    const std::vector<VertexData>& data,
    const std::vector<Magnum::UnsignedInt>&& indices) :
    meshIndices(std::move(indices)) {

    for (const auto& d : data) {
        meshVertices.push_back(d.position);
        meshNormals.push_back(d.normal);
        meshTexcoords.push_back(d.texcoord);
    }
    meshIndices = indices;
}
        
const std::vector<IndexedMeshData::VertexData> IndexedMeshData::getVertexData() const {
    std::vector<VertexData> ret(meshVertices.size());
    for (int i = 0; i < meshVertices.size(); ++i) {
        ret[i].position = meshVertices[i];
        ret[i].normal = meshNormals[i];
        ret[i].texcoord = meshTexcoords[i];
    }
    return ret;
}

const std::vector<Magnum::UnsignedInt>& IndexedMeshData::getIndices() const {
    return meshIndices;
}

const std::vector<Magnum::Vector3>& IndexedMeshData::getVertices() const {
    return meshVertices;
}

const std::vector<Magnum::Vector3>& IndexedMeshData::getNormals() const {
    return meshNormals;
}

const std::vector<Magnum::Vector2>& IndexedMeshData::getTexcoords() const {
    return meshTexcoords;
}
