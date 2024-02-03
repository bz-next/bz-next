#include "WorldRenderer.h"
#include "Drawables.h"
#include "Magnum/MeshTools/GenerateLines.h"
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
#include <Magnum/MeshTools/CompileLines.h>
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

SceneGraph::DrawableGroup3D *WorldRenderer::getDebugDrawableGroup()
{
    return worldDebugDrawables;
}

Object3D *WorldRenderer::getWorldObject()
{
    return worldParent;
}

WorldRenderer::WorldRenderer() {
    worldDrawables = NULL;
    worldTransDrawables = NULL;
    worldDebugDrawables = NULL;
    worldParent = NULL;
    debugLine = NULL;
}

WorldRenderer::~WorldRenderer() {
    destroyWorldObject();
}

void WorldRenderer::setExcludeSet(std::set<std::string> matnames) {
    materialsToExclude = matnames;
}

void WorldRenderer::clearExcludeSet() {
    materialsToExclude.clear();
}

void WorldRenderer::createWorldObject(const WorldSceneBuilder *sb) {
    worldDrawables = new SceneGraph::DrawableGroup3D{};
    worldTransDrawables = new SceneGraph::DrawableGroup3D{};
    worldDebugDrawables = new SceneGraph::DrawableGroup3D{};
    worldParent = new Object3D{};
    worldParent->scale({0.05, 0.05, 0.05});

    {
        debugLine = new GL::Mesh{
            MeshTools::compileLines(
                MeshTools::generateLines(
                    WorldPrimitiveGenerator::debugLine(
                        {-1.0f, 0.0f, 0.0f},
                        {1.0f, 0.0f, 0.0f})))};
        auto x = 
                    WorldPrimitiveGenerator::debugLine(
                        {-1.0f, 0.0f, 0.0f},
                        {1.0f, 0.0f, 0.0f});
        Object3D *debugLines = new Object3D{};
        debugLines->setParent(worldParent);
        debugLines->scale(Vector3{100.0, 100.0, 100.0});
        // Create debug grid
        int numLines = 20;
        float start = -1.0f;
        float step = 2.0f/numLines;

        // xy grid
        for (int i = 0; i <= numLines; ++i) {
            Object3D *xline = new Object3D{};
            xline->setParent(debugLines);
            xline->translate({0.0f, start+i*step, 0.0f});
            new DebugLineDrawable{*xline, _lineShader, 0.5*Color3{1.0f, 0.0f, 0.0f}, *debugLine, *worldDebugDrawables};
            Object3D *yline = new Object3D{};
            yline->setParent(debugLines);
            yline->rotateZ(Deg(90.0f));
            yline->translate({start+i*step, 0.0f, 0.0f});
            new DebugLineDrawable{*yline, _lineShader, 0.5*Color3{0.0f, 1.0f, 0.0f}, *debugLine, *worldDebugDrawables};
        }
        Object3D *zline = new Object3D{};
        zline->setParent(debugLines);
        zline->rotateY(Deg(90.0));
        new DebugLineDrawable{*zline, _lineShader, 0.5*Color3{0.0f, 0.0f, 1.0f}, *debugLine, *worldDebugDrawables};
    }

    std::vector<std::string> matnames = sb->getMaterialList();
    // Render opaque objects first
    for (const auto& matname: matnames) {
        auto *mat = MAGNUMMATERIALMGR.findMaterial(matname);
        if (mat) {
            if (mat->getDiffuse()[3] < 0.999f) continue;
        }
        if (materialsToExclude.find(matname) != materialsToExclude.end()) {
            continue;
        }
        Object3D *matobjs = new Object3D;
        std::string entryName = "mat_" + matname;
        worldMeshes[entryName].emplace_back(std::move(sb->compileMatMesh(matname)));
        GL::Mesh *m = &worldMeshes[entryName].back();
        matobjs->setParent(worldParent);
        new BZMaterialDrawable(*matobjs, matShader, matShaderUntex, *m, mat, *worldDrawables);
    }
    // Render transparent objects second
    for (const auto& matname: matnames) {
        auto *mat = MAGNUMMATERIALMGR.findMaterial(matname);
        if (mat) {
            if (mat->getDiffuse()[3] >= 0.999f) continue;
        }
        if (materialsToExclude.find(matname) != materialsToExclude.end()) {
            continue;
        }
        Object3D *matobjs = new Object3D;
        std::string entryName = "mat_" + matname;
        worldMeshes[entryName].emplace_back(std::move(sb->compileMatMesh(matname)));
        GL::Mesh *m = &worldMeshes[entryName].back();
        matobjs->setParent(worldParent);
        new BZMaterialDrawable(*matobjs, matShader, matShaderUntex, *m, mat, *worldTransDrawables);
    }
}

void WorldRenderer::destroyWorldObject() {
    if (worldDrawables) delete worldDrawables;
    if (worldTransDrawables) delete worldTransDrawables;
    if (worldDebugDrawables) delete worldDebugDrawables;
    if (worldParent) delete worldParent;
    if (debugLine) delete debugLine;
    worldMeshes.clear();
    worldDrawables = NULL;
    worldTransDrawables = NULL;
    worldDebugDrawables = NULL;
    worldParent = NULL;
    debugLine = NULL;
}