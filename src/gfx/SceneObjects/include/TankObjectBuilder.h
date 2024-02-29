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

#include "TankSceneObject.h"

class TankObjectBuilder {
    public:
        static TankObjectBuilder& instance() { static TankObjectBuilder instance; return instance; }
        void setPlayerID(int playerID);
        void setTeam(TeamColor tc);
        void setDrawableGroup(Magnum::SceneGraph::DrawableGroup3D* dgrp);
        TankSceneObject* buildTank();

        void cleanup();
    private:
        TankObjectBuilder() : _meshes{TankSceneObject::LastTankPart} {}
        int _playerID;
        TeamColor _tc;
        Magnum::SceneGraph::DrawableGroup3D* _dgrp = NULL;

        void prepareMaterials();
        void loadTankMesh();

        Corrade::Containers::Array<Magnum::GL::Mesh> _meshes;
        bool _isMeshLoaded = false;
};

#endif
