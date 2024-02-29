#include "TankObjectBuilder.h"
#include "Corrade/Tags.h"
#include "TankSceneObject.h"
#include <Magnum/SceneGraph/SceneGraph.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/MeshTools/Compile.h>

#include <Corrade/Utility/Resource.h>
#include <string>

#include "MagnumDefs.h"

#include "Drawables.h"

#include "MagnumBZMaterial.h"

#include "TextureMatrix.h"
#include "bzfio.h"
#include "global.h"

using namespace Magnum;
using namespace Magnum::SceneGraph;

void TankObjectBuilder::setTeam(TeamColor tc) {
    _tc = tc;
}

void TankObjectBuilder::setPlayerID(int playerID) {
    _playerID = playerID;
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

    if (importer->meshCount() != TankSceneObject::LastTankPart) {
        logDebugMessage(1, "Tank obj contains %d meshes (expected %d)\n", importer->meshCount(), TankSceneObject::LastTankPart);
    }

    for (int i = 0; i < TankSceneObject::LastTankPart; ++i) {
        int id = importer->meshForName(TankSceneObject::ObjLabels[i]);
        if (id == -1) {
            logDebugMessage(1, "Missing mesh %s\n", TankSceneObject::ObjLabels[i]);
            continue;
        }
        Containers::Optional<Trade::MeshData> meshData;
        if (!(meshData = importer->mesh(id))) {
            logDebugMessage(1, "Could not load mesh %s\n", TankSceneObject::ObjLabels[i]);
            continue;
        }
        _meshes[i] = MeshTools::compile(*meshData);
    }
    _isMeshLoaded = true;
}

void TankObjectBuilder::prepareMaterials() {
    if (!Team::isPlayableTeam(_tc)) {
        logDebugMessage(1, "Tried to generate material for non-playable team %d\n", _tc);
        return;
    }
    // Names for tank body materials are _Player<ID>Tank
    std::string tankmatname = "_Player" + std::to_string(_playerID) + "Tank";
    // Check if it already exists. If not, add it.
    if (MAGNUMMATERIALMGR.findMaterial(tankmatname) == NULL) {
        auto* tankmat = new MagnumBZMaterial();
        tankmat->setName(tankmatname);
        std::string texname = Team::getImagePrefix(_tc) + "tank";
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
        TextureMatrix *tm = new TextureMatrix;
        tm->setName(treadLeftName);
        //tm->setDynamicShift(0.2f, 0.0f);
        tm->finalize();
        int id = TEXMATRIXMGR.addMatrix(tm);
        mat->setTextureMatrix(id);
        MAGNUMMATERIALMGR.addMaterial(mat);
    }
    if (MAGNUMMATERIALMGR.findMaterial(treadRightName) == NULL) {
        auto *mat = new MagnumBZMaterial();
        mat->setName(treadRightName);
        mat->addTexture("treads");
        TextureMatrix *tm = new TextureMatrix;
        tm->setName(treadRightName);
        //tm->setDynamicShift(-0.2f, 0.0f);
        tm->finalize();
        int id = TEXMATRIXMGR.addMatrix(tm);
        mat->setTextureMatrix(id);
        MAGNUMMATERIALMGR.addMaterial(mat);
    }
    // Add wheel materials if they don't already exist
    std::string wheelPrefix = "_Player" + std::to_string(_playerID) + "Wheel";
    std::string wheelLeftName = wheelPrefix + "Left";
    std::string wheelRightName = wheelPrefix + "Right";
    if (MAGNUMMATERIALMGR.findMaterial(wheelLeftName) == NULL) {
        auto *mat = new MagnumBZMaterial();
        mat->setName(wheelLeftName);
        std::string texname = Team::getImagePrefix(_tc) + "tank";
        mat->addTexture(texname);
        TextureMatrix *tm = new TextureMatrix;
        tm->setName(wheelLeftName);
        //tm->setDynamicShift(0.2f, 0.0f);
        tm->finalize();
        int id = TEXMATRIXMGR.addMatrix(tm);
        mat->setTextureMatrix(id);
        MAGNUMMATERIALMGR.addMaterial(mat);
    }
    if (MAGNUMMATERIALMGR.findMaterial(wheelRightName) == NULL) {
        auto *mat = new MagnumBZMaterial();
        mat->setName(wheelRightName);
        std::string texname = Team::getImagePrefix(_tc) + "tank";
        mat->addTexture(texname);
        TextureMatrix *tm = new TextureMatrix;
        tm->setName(wheelRightName);
        //tm->setDynamicShift(-0.2f, 0.0f);
        tm->finalize();
        int id = TEXMATRIXMGR.addMatrix(tm);
        mat->setTextureMatrix(id);
        MAGNUMMATERIALMGR.addMaterial(mat);
    }
    // Add barrel material if it doesn't already exist
    std::string barrelmatname = "_Player" + std::to_string(_playerID) + "TankBarrel";
    if (MAGNUMMATERIALMGR.findMaterial(barrelmatname) == NULL) {
        auto *mat = new MagnumBZMaterial();
        mat->setName(barrelmatname);
        float color[] = {0.4f, 0.4f, 0.4f, 1.0f};
        mat->setDiffuse(color);
        MAGNUMMATERIALMGR.addMaterial(mat);
    }
}

TankSceneObject* TankObjectBuilder::buildTank() {
    if (_dgrp == NULL) return NULL;

    if (!_isMeshLoaded) loadTankMesh();
    prepareMaterials();

    auto bodymat = MAGNUMMATERIALMGR.findMaterial("_Player" + std::to_string(_playerID) + "Tank");
    auto barrelmat = MAGNUMMATERIALMGR.findMaterial("_Player" + std::to_string(_playerID) + "TankBarrel");
    auto treadLeftMat = MAGNUMMATERIALMGR.findMaterial("_Player" + std::to_string(_playerID) + "TreadLeft");
    auto treadRightMat = MAGNUMMATERIALMGR.findMaterial("_Player" + std::to_string(_playerID) + "TreadRight");
    auto wheelLeftMat = MAGNUMMATERIALMGR.findMaterial("_Player" + std::to_string(_playerID) + "WheelLeft");
    auto wheelRightMat = MAGNUMMATERIALMGR.findMaterial("_Player" + std::to_string(_playerID) + "WheelRight");
    

    TankSceneObject *tank = new TankSceneObject{};
    
    Object3D *body = new Object3D;
    body->setParent(tank);
    new BZMaterialDrawable{*body, _meshes[0], bodymat, *_dgrp};
    tank->_parts[TankSceneObject::Body] = body;

    Object3D *barrel = new Object3D;
    barrel->setParent(tank);
    new BZMaterialDrawable{*barrel, _meshes[1], barrelmat, *_dgrp};
    tank->_parts[TankSceneObject::Barrel] = barrel;

    Object3D *turret = new Object3D;
    turret->setParent(tank);
    new BZMaterialDrawable{*turret, _meshes[2], bodymat, *_dgrp};
    tank->_parts[TankSceneObject::Turret] = turret;

    Object3D *leftcasing = new Object3D;
    leftcasing->setParent(tank);
    new BZMaterialDrawable{*leftcasing, _meshes[3], bodymat, *_dgrp};
    tank->_parts[TankSceneObject::LeftCasing] = leftcasing;

    Object3D *rightcasing = new Object3D;
    rightcasing->setParent(tank);
    new BZMaterialDrawable{*rightcasing, _meshes[4], bodymat, *_dgrp};
    tank->_parts[TankSceneObject::RightCasing] = rightcasing;

    Object3D *lefttread = new Object3D;
    lefttread->setParent(tank);
    new BZMaterialDrawable{*lefttread, _meshes[5], treadLeftMat, *_dgrp};
    tank->_parts[TankSceneObject::LeftTread] = lefttread;

    Object3D *righttread = new Object3D;
    righttread->setParent(tank);
    new BZMaterialDrawable{*righttread, _meshes[6], treadRightMat, *_dgrp};
    tank->_parts[TankSceneObject::RightTread] = righttread;

    // The wheels
    for (int i = TankSceneObject::LeftWheel0; i <= TankSceneObject::RightWheel3; ++i) {
        Object3D *w = new Object3D;
        w->setParent(tank);
        new BZMaterialDrawable{*w, _meshes[i], (i <= TankSceneObject::LeftWheel3) ? wheelLeftMat : wheelRightMat, *_dgrp};
        tank->_parts[i] = w;
    }

    tank->_bodyMat = bodymat;
    tank->_lTreadMat = treadLeftMat;
    tank->_rTreadMat = treadRightMat;
    tank->_barrelMat = barrelmat;
    tank->_lWheelMat = wheelLeftMat;
    tank->_rWheelMat = wheelRightMat;
    tank->_team = _tc;

    return tank;
    
}

void TankObjectBuilder::cleanup() {
    for (int i = 0; i < TankSceneObject::LastTankPart; ++i) {
        _meshes[i] = GL::Mesh{NoCreate};
    }
    _isMeshLoaded = false;
}