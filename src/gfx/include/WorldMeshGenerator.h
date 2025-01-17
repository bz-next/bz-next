#ifndef WORLDSCENEBUILDER_H
#define WORLDSCENEBUILDER_H

#include <map>
#include <string>
#include <list>
#include <vector>
#include <utility>

#include <Magnum/GL/Mesh.h>

#include "IndexedMeshData.h"


#include "BoxBuilding.h"
#include "PyramidBuilding.h"
#include "BaseBuilding.h"
#include "WallObstacle.h"
#include "Teleporter.h"

// The approach here results in multiple copies of map mesh data in RAM
// This is inefficient, but hopefully a map isn't so big that this leads
// to memory use issues

// Pair material name with mesh data
typedef std::pair<std::string, IndexedMeshData> MatMesh;

// Contains all the meshes that make up a world object
// These may be later compiled into combined meshes,
// or rendered individually. Rendering individually is
// useful for object picking.
class WorldObject {
    public:
    void addMatMesh(std::string materialname, IndexedMeshData&& md);
    const std::vector<MatMesh>& getMatMeshes() const;
    private:
    std::vector<MatMesh> matMeshes;
};

class WorldMeshGenerator {
    public:
    void addBox(BoxBuilding& o);
    void addPyr(PyramidBuilding& o);
    void addBase(BaseBuilding& o);
    void addWall(WallObstacle& o);
    void addTeleporter(const Teleporter& o);
    void addGround(float worldSize);
    void addMesh(const MeshObstacle& o);

    // Empty it out to get ready to load a new map
    void reset();

    Magnum::GL::Mesh compileMatMesh(std::string matname) const;
    std::vector<std::string> getMaterialList() const;
    private:
    std::vector<WorldObject> worldObjects;
};

#endif
