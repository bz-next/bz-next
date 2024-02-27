#ifndef TANKOBJECTBUILDER_H
#define TANKOBJECTBUILDER_H

#include <Magnum/SceneGraph/AnimableGroup.h>
#include <Magnum/SceneGraph/Drawable.h>

#include <Magnum/GL/Mesh.h>

#include <Magnum/SceneGraph/SceneGraph.h>

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>

#include "MagnumDefs.h"
#include "Team.h"

class TankObjectBuilder {
    enum TankPart
    {
        Body = 0,
        Barrel,
        Turret,
        LeftCasing,
        RightCasing,

        LeftTread, // animated parts
        RightTread,
        LeftWheel0,
        LeftWheel1,
        LeftWheel2,
        LeftWheel3,
        RightWheel0,
        RightWheel1,
        RightWheel2,
        RightWheel3,

        LastTankPart
    };

    static constexpr const char* ObjLabels[] = {
        "Body",
        "Barrel",
        "Turret",
        "LeftCasing",
        "RightCasing",
        "LeftTread",
        "RightTread",
        "LeftWheel0",
        "LeftWheel1",
        "LeftWheel2",
        "LeftWheel3",
        "RightWheel0",
        "RightWheel1",
        "RightWheel2",
        "RightWheel3",
    };
    
    public:
        TankObjectBuilder() : _meshes{LastTankPart} {}
        void setPlayerID(int playerID);
        void setTeam(TeamColor tc);
        void setAnimableGroup(Magnum::SceneGraph::AnimableGroup3D* agrp);
        void setDrawableGroup(Magnum::SceneGraph::DrawableGroup3D* dgrp);
        Object3D* buildTank();
    private:
        int _playerID;
        TeamColor _tc;
        Magnum::SceneGraph::AnimableGroup3D* _agrp = NULL;
        Magnum::SceneGraph::DrawableGroup3D* _dgrp = NULL;

        void prepareMaterials();
        void loadTankMesh();

        Corrade::Containers::Array<Magnum::GL::Mesh> _meshes;
};

#endif
