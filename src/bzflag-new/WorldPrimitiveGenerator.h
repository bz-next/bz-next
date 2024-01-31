#ifndef WORLDPRIMITIVEGENERATOR_H
#define WORLDPRIMITIVEGENERATOR_H

#include <vector>

#include <Magnum/Trade/MeshData.h>
#include <Magnum/Math/Vector3.h>

class WorldPrimitiveGenerator {
    public:
        WorldPrimitiveGenerator() = delete;

        static Magnum::Trade::MeshData quad(const float base[3], const float uEdge[3], const float vEdge[3], float uOffset, float vOffset, float uRepeats, float vRepeats);
        static Magnum::Trade::MeshData tri(const float base[3], const float uEdge[3], const float vEdge[3], float uOffset, float vOffset, float uRepeats, float vRepeats);
        static Magnum::Trade::MeshData planarPoly(const std::vector<Magnum::Math::Vector3<float>> &verts, const std::vector<Magnum::Math::Vector3<float>> &norms, const std::vector<Magnum::Math::Vector2<float>> &texcoords);
        static Magnum::Trade::MeshData debugLine(Magnum::Math::Vector3<float> a, Magnum::Math::Vector3<float> b);
};

#endif