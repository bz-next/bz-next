#include <Magnum/Trade/MeshData.h>

class WorldPrimitiveGenerator {
    public:
        WorldPrimitiveGenerator() = delete;

        static Magnum::Trade::MeshData pyrSolid();
        static Magnum::Trade::MeshData quad(const float base[3], const float uEdge[3], const float vEdge[3], float uOffset, float vOffset, float uRepeats, float vRepeats);
        static Magnum::Trade::MeshData tri(const float base[3], const float uEdge[3], const float vEdge[3], float uOffset, float vOffset, float uRepeats, float vRepeats);
        static Magnum::Trade::MeshData wall();
};
