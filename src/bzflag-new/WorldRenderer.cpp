#include "WorldRenderer.h"
#include "Drawables.h"
#include "Magnum/Trade/MaterialData.h"
#include "MagnumBZMaterial.h"
#include "WorldPrimitiveGenerator.h"

#include "Obstacle.h"
#include "ObstacleList.h"
#include "ObstacleMgr.h"

#include "Corrade/Containers/ArrayView.h"
#include "WorldSceneBuilder.h"
#include <Magnum/Trade/MeshData.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/MeshTools/RemoveDuplicates.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/PhongGL.h>

using namespace Magnum;
using namespace Magnum::Math::Literals;

SceneGraph::DrawableGroup3D *WorldRenderer::getDrawableGroup()
{
    return worldDrawables;
}
SceneGraph::DrawableGroup3D *WorldRenderer::getTransDrawableGroup()
{
    return worldTransDrawables;
}

Object3D *WorldRenderer::getWorldObject()
{
    return worldParent;
}

WorldRenderer::WorldRenderer() {
    worldDrawables = NULL;
    worldTransDrawables = NULL;
    worldParent = NULL;
}

WorldRenderer::~WorldRenderer() {
    if (worldDrawables) delete worldDrawables;
    if (worldTransDrawables) delete worldTransDrawables;
    if (worldParent) delete worldParent;
}

void WorldRenderer::createWorldObject(const WorldSceneBuilder *sb) {
    worldDrawables = new SceneGraph::DrawableGroup3D{};
    worldTransDrawables = new SceneGraph::DrawableGroup3D{};
    worldParent = new Object3D{};
    worldParent->scale({0.05, 0.05, 0.05});

    std::vector<std::string> matnames = sb->getMaterialList();
    // Render opaque objects first
    for (const auto& matname: matnames) {
        // TODO: SUPER slow. Fix this
        if (auto *m = MAGNUMMATERIALMGR.findMaterial(matname)) {
            if (m->getDiffuse()[3] < 0.999f) continue;
        }
        Object3D *matobjs = new Object3D;
        std::string entryName = "mat_" + matname;
        worldMeshes[entryName].push_back(MeshTools::compile(sb->compileMatMesh(matname)));
        GL::Mesh *m = &worldMeshes[entryName].back();
        matobjs->setParent(worldParent);
        new BZMaterialDrawable(*matobjs, matShader, matShaderUntex, *m, matname, *worldDrawables);
    }
    // Render transparent objects second
    for (const auto& matname: matnames) {
        if (auto *m = MAGNUMMATERIALMGR.findMaterial(matname)) {
            if (m->getDiffuse()[3] >= 0.999f) continue;
        }
        Object3D *matobjs = new Object3D;
        std::string entryName = "mat_" + matname;
        worldMeshes[entryName].push_back(MeshTools::compile(sb->compileMatMesh(matname)));
        GL::Mesh *m = &worldMeshes[entryName].back();
        matobjs->setParent(worldParent);
        new BZMaterialDrawable(*matobjs, matShader, matShaderUntex, *m, matname, *worldTransDrawables);
    }
}

void WorldRenderer::destroyWorldObject() {
    if (worldDrawables) delete worldDrawables;
    if (worldParent) delete worldParent;
    worldDrawables = NULL;
    worldParent = NULL;
}