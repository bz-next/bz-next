#ifndef INDEXEDMESHDATA_H
#define INDEXEDMESHDATA_H

#include <vector>
#include "Magnum/Math/Vector3.h"
#include "Magnum/Math/Vector2.h"
#include "Magnum/Types.h"

class IndexedMeshData {
    public:
        struct VertexData {
            Magnum::Vector3 position;
            Magnum::Vector3 normal;
            Magnum::Vector2 texcoord;
        };

        IndexedMeshData(
            const std::vector<Magnum::Math::Vector3<float>> &&verts,
            const std::vector<Magnum::Math::Vector3<float>> &&norms,
            const std::vector<Magnum::Math::Vector2<float>> &&texcoords,
            const std::vector<Magnum::UnsignedInt> &&indices);

        IndexedMeshData(
            const std::vector<VertexData>& data,
            const std::vector<Magnum::UnsignedInt>&& indices);
        
        const std::vector<VertexData> getVertexData() const;
        const std::vector<Magnum::Vector3>& getVertices() const;
        const std::vector<Magnum::Vector3>& getNormals() const;
        const std::vector<Magnum::Vector2>& getTexcoords() const;
        const std::vector<Magnum::UnsignedInt>& getIndices() const;
    
    private:
        std::vector<Magnum::Math::Vector3<float>> meshVertices;
        std::vector<Magnum::Math::Vector3<float>> meshNormals;
        std::vector<Magnum::Math::Vector2<float>> meshTexcoords;
        std::vector<Magnum::UnsignedInt> meshIndices;
};

#endif
