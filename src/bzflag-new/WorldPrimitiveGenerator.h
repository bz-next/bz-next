#include <Magnum/Trade/MeshData.h>

class WorldPrimitiveGenerator {
    public:
        WorldPrimitiveGenerator() = delete;

        static Magnum::Trade::MeshData pyrSolid();
        static Magnum::Trade::MeshData wall();
};
