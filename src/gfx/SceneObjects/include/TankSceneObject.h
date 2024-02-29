#ifndef TANKSCENEOBJECT_H
#define TANKSCENEOBJECT_H

#include <Magnum/SceneGraph/Drawable.h>

#include "Magnum/SceneGraph/SceneGraph.h"
#include "Magnum/Types.h"
#include "MagnumBZMaterial.h"
#include "MagnumDefs.h"
#include "global.h"

#include <cstring>

// This is partially a port from TankSceneNode.
// It is ugly to have the Player class poke and prod at various state in the node
// But it's the way it was done before, and for the sake of getting things working,
// we just replicate that interface.
class TankSceneObject : public Object3D {
    friend class TankObjectBuilder;
    public:
    explicit TankSceneObject():
        Object3D{} {}
    virtual ~TankSceneObject() {}

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

    enum TankSize
    {
        Normal = 0,
        Obese,
        Tiny,
        Narrow,
        Thief,
        LastTankSize
    };

    void setObese() { _tankSize = Obese; _useDimensions = false; }
    void setTiny() { _tankSize = Tiny; _useDimensions = false; }
    void setNarrow() { _tankSize = Narrow; _useDimensions = false; }
    void setThief() { _tankSize = Thief; _useDimensions = false; }
    void setNormal() { _tankSize = Normal; _useDimensions = false; }
    void setDimensions(const float size[3]) { _tankSize = Normal; memcpy(_dimensions, size, sizeof(float[3])); _useDimensions = true; }
    void setPosition(const float pos[3], const float angleRad);
    void addTreadOffsets(float left, float right);
    void setExplodeFraction(float t);
    void resetExplosion() { _isExploding = false; _explodeFraction = 0.0f; }

    bool isExploding() const { return _isExploding; }

    void rebuildExplosion();

    void setColor(const float rgba[4]);
    void setTeam(TeamColor team);
    
    private:

        Object3D* _parts[LastTankPart];
        MagnumBZMaterial *_bodyMat, *_lTreadMat, *_rTreadMat, *_barrelMat, *_lWheelMat, *_rWheelMat;
        TeamColor _team;

        float _dimensions[3];
        float _leftTreadOffset = 0.0f;
        float _rightTreadOffset = 0.0f;
        float _leftWheelOffset;
        float _rightWheelOffset;
        bool _useDimensions;
        float _explodeFraction;
        bool _isExploding = false;

        float _color[4];
        TankSize _tankSize;

        // For explosion
        float _spin[LastTankPart][4];
        float _vel[LastTankPart][3];

};

#endif
