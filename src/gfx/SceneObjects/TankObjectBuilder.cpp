#include "TankObjectBuilder.h"
#include <Magnum/SceneGraph/SceneGraph.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/MeshTools/Compile.h>

#include <Corrade/Utility/Resource.h>

#include "MagnumDefs.h"

#include "Drawables.h"

#include "MagnumBZMaterial.h"

#include "bzfio.h"
#include "global.h"

using namespace Magnum;
using namespace Magnum::SceneGraph;

void TankObjectBuilder::setTeam(TeamColor tc) {
    _tc = tc;
}

void TankObjectBuilder::setAnimableGroup(AnimableGroup3D* agrp) {
    _agrp = agrp;
}

void TankObjectBuilder::setDrawableGroup(DrawableGroup3D* dgrp) {
    _dgrp = dgrp;
}

void TankObjectBuilder::loadTankMesh() {
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate("ObjImporter");

    Utility::Resource rs{"bzflag-tank-data"};

    const std::string tankfilename = "tank.obj";

    if (!rs.hasFile(tankfilename)) {
        logDebugMessage(1, "Model not found in packed resources: %s\n", tankfilename.c_str());
        return;
    }

    if (!importer->openData(rs.getRaw(tankfilename))) {
        logDebugMessage(1, "Could not import %s from packed resources.\n", tankfilename.c_str());
        return;
    }

    if (importer->meshCount() != LastTankPart) {
        logDebugMessage(1, "Tank obj contains %d meshes (expected %d)\n", importer->meshCount(), LastTankPart);
    }

    for (int i = 0; i < LastTankPart; ++i) {
        int id = importer->meshForName(ObjLabels[i]);
        if (id == -1) {
            logDebugMessage(1, "Missing mesh %s\n", ObjLabels[i]);
            continue;
        }
        Containers::Optional<Trade::MeshData> meshData;
        if (!(meshData = importer->mesh(id))) {
            logDebugMessage(1, "Could not load mesh %s\n", ObjLabels[i]);
            continue;
        }
        _meshes[i] = MeshTools::compile(*meshData);
    }
}

void TankObjectBuilder::prepareMaterials() {
    if (!Team::isColorTeam(_tc)) {
        // Might still be rabbit or hunter or rogue
        if (_tc != RabbitTeam && _tc != HunterTeam && _tc != RogueTeam) {
            logDebugMessage(1, "Tried to generate material for non-playable team %d\n", _tc);
            return;
        }
    }
    // Names for tank body materials are _<teamPrefix>Tank
    std::string tankmatname = "_" + Team::getImagePrefix(_tc) + "Tank";
    // Check if it already exists. If not, add it.
    if (MAGNUMMATERIALMGR.findMaterial(tankmatname) == NULL) {
        auto* tankmat = new MagnumBZMaterial();
        tankmat->setName(tankmatname);
        std::string texname = Team::getImagePrefix(_tc) + "_tank";
        tankmat->addTexture(texname);
        MAGNUMMATERIALMGR.addMaterial(tankmat);
    }
    // Add tread materials if they don't already exist
    std::string treadPrefix = "_Player" + std::to_string(_playerID) + "Tread";
    std::string treadLeftName = treadPrefix + "Left";
    std::string treadRightName = treadPrefix + "Right";
    if (MAGNUMMATERIALMGR.findMaterial(treadLeftName) == NULL) {
        auto *mat = new MagnumBZMaterial();
        mat->setName(treadLeftName);
        mat->addTexture("treads");
        MAGNUMMATERIALMGR.addMaterial(mat);
    }
    if (MAGNUMMATERIALMGR.findMaterial(treadRightName) == NULL) {
        auto *mat = new MagnumBZMaterial();
        mat->setName(treadRightName);
        mat->addTexture("treads");
        MAGNUMMATERIALMGR.addMaterial(mat);
    }
    // Add barrel material if it doesn't already exist
    std::string barrelmatname = "_TankBarrel";
    if (MAGNUMMATERIALMGR.findMaterial(barrelmatname) == NULL) {
        auto *mat = new MagnumBZMaterial();
        mat->setName(barrelmatname);
        float color[] = {0.4f, 0.4f, 0.4f, 1.0f};
        mat->setDiffuse(color);
        MAGNUMMATERIALMGR.addMaterial(mat);
    }
}

Object3D* TankObjectBuilder::buildTank() {
    if (_dgrp == NULL || _agrp == NULL) return NULL;
    prepareMaterials();

    auto bodymat = MAGNUMMATERIALMGR.findMaterial("_" + Team::getImagePrefix(_tc) + "Tank");
    auto barrelmat = MAGNUMMATERIALMGR.findMaterial("_TankBarrel");
    auto treadLeftMat = MAGNUMMATERIALMGR.findMaterial("_Player" + std::to_string(_playerID) + "TreadLeft");
    auto treadRightMat = MAGNUMMATERIALMGR.findMaterial("_Player" + std::to_string(_playerID) + "TreadRight");

    Object3D *tank = new Object3D;
    
    Object3D *body = new Object3D;
    body->setParent(tank);
    new BZMaterialDrawable{*body, _meshes[0], bodymat, *_dgrp};

    Object3D *barrel = new Object3D;
    barrel->setParent(tank);
    new BZMaterialDrawable{*barrel, _meshes[1], barrelmat, *_dgrp};

    Object3D *turret = new Object3D;
    turret->setParent(tank);
    new BZMaterialDrawable{*turret, _meshes[2], bodymat, *_dgrp};

    Object3D *leftcasing = new Object3D;
    leftcasing->setParent(tank);
    new BZMaterialDrawable{*leftcasing, _meshes[3], bodymat, *_dgrp};

    Object3D *rightcasing = new Object3D;
    rightcasing->setParent(tank);
    new BZMaterialDrawable{*rightcasing, _meshes[4], bodymat, *_dgrp};

    Object3D *lefttread = new Object3D;
    lefttread->setParent(tank);
    new BZMaterialDrawable{*lefttread, _meshes[5], treadLeftMat, *_dgrp};

    Object3D *righttread = new Object3D;
    righttread->setParent(tank);
    new BZMaterialDrawable{*righttread, _meshes[6], treadRightMat, *_dgrp};

    // The wheels
    for (int i = LeftWheel0; i <= RightWheel3; ++i) {
        Object3D *w = new Object3D;
        w->setParent(tank);
        new BZMaterialDrawable{*w, _meshes[i], bodymat, *_dgrp};
    }

    return tank;
    
}