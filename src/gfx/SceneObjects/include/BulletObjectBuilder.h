#ifndef BULLETOBJECTBUILDER_H
#define BULLETOBJECTBUILDER_H

#include <Magnum/SceneGraph/AnimableGroup.h>
#include <Magnum/SceneGraph/Drawable.h>

#include <Magnum/GL/Mesh.h>

#include <Magnum/SceneGraph/SceneGraph.h>

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>

#include "MagnumDefs.h"
#include "Team.h"

#include "BulletSceneObject.h"

class BulletObjectBuilder {
    public:
        static BulletObjectBuilder& instance() { static BulletObjectBuilder instance; return instance; }
        void setTeam(TeamColor tc);
        void setDrawableGroup(Magnum::SceneGraph::DrawableGroup3D* dgrp);
        BulletSceneObject* buildBullet();

        void cleanup();
    private:
        BulletObjectBuilder() {}
        TeamColor _tc;
        Magnum::SceneGraph::DrawableGroup3D* _dgrp = NULL;

        void prepareMaterials();

        Magnum::GL::Mesh _mesh;
};

#endif
