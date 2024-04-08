#ifndef BULLETSCENEOBJECT_H
#define BULLETSCENEOBJECT_H

#include <Magnum/SceneGraph/Drawable.h>

#include "Magnum/SceneGraph/SceneGraph.h"
#include "Magnum/Types.h"
#include "MagnumBZMaterial.h"
#include "MagnumDefs.h"
#include "global.h"

#include <cstring>

class BulletSceneObject : public Object3D {
    friend class BulletObjectBuilder;
    public:
    explicit BulletSceneObject():
        Object3D{} {}
    virtual ~BulletSceneObject() {}

    void setPosition(const float pos[3], const float angleRad);

    bool isExploding() const { return _isExploding; }

    void setTeam(TeamColor team);
    
    private:

        MagnumBZMaterial *_bulletMat;
        TeamColor _team;

        float _explodeFraction = 0.0f;
        bool _isExploding = false;

        float _color[4];
};

#endif
