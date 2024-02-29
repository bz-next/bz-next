#include "TankSceneObject.h"

#include "common.h"

#include "Team.h"

using namespace Magnum;

void TankSceneObject::setPosition(const float pos[3], const float angleRad) {
    Object3D::resetTransformation();
    Object3D::rotateZ(Math::Rad<float>(angleRad));
    Object3D::translate({pos[0], pos[1], pos[2]});
}

void TankSceneObject::addTreadOffsets(float left, float right) {
    // These all need magic multiplication factors so that they look
    // roughly correct. Do that later... Just pick factors that look OK.
    _leftTreadOffset += left;
    _rightTreadOffset += right;
    for (int i = LeftWheel0; i <= LeftWheel3; ++i) {
        _parts[i]->rotateY(Math::Rad<float>(left));
    }
    for (int i = RightWheel0; i <= RightWheel3; ++i) {
        _parts[i]->rotateY(Math::Rad<float>(right));
    }
}

void TankSceneObject::setExplodeFraction(float t) {
    _explodeFraction = t;
    for (int i = 0; i < LastTankPart; ++i) {
        _parts[i]->resetTransformation();
        _parts[i]->rotate(Math::Rad<float>(t*_spin[i][3]), {_spin[i][0], _spin[i][1], _spin[i][2]});
        _parts[i]->translate({t*_vel[i][0], t*_vel[i][1], t*_vel[i][2]});
    }
}

void TankSceneObject::rebuildExplosion() {
    const float maxExplosionVel = 40.0f;
    const float vertExplosionRatio = 0.5f;
    // prepare explosion rotations and translations
    for (int i = 0; i < LastTankPart; i++)
    {
        // pick an unbiased rotation vector
        float d;
        do
        {
            _spin[i][0] = (float)bzfrand() - 0.5f;
            _spin[i][1] = (float)bzfrand() - 0.5f;
            _spin[i][2] = (float)bzfrand() - 0.5f;
            d = hypotf(_spin[i][0], hypotf(_spin[i][1], _spin[i][2]));
        }
        while (d < 0.001f || d > 0.5f);
        _spin[i][0] /= d;
        _spin[i][1] /= d;
        _spin[i][2] /= d;

        // now an angular velocity -- make sure we get at least 2 complete turns
        _spin[i][3] = 360.0f * (5.0f * (float)bzfrand() + 2.0f);

        // cheezy spheroid explosion pattern
        const float vhMax = maxExplosionVel;
        const float vhMag = vhMax * sinf((float)(M_PI * 0.5 * bzfrand()));
        const float vhAngle = (float)(2.0 * M_PI * bzfrand());
        _vel[i][0] = cosf(vhAngle) * vhMag;
        _vel[i][1] = sinf(vhAngle) * vhMag;
        const float vz = sqrtf(fabsf((vhMax*vhMax) - (vhMag*vhMag)));
        _vel[i][2] = vz * vertExplosionRatio; // flatten it a little
        if (bzfrand() > 0.5)
            _vel[i][2] = -_vel[i][2];
    }
}

void TankSceneObject::setColor(const float rgba[4]) {
    _bodyMat->setDiffuse(rgba);
    _lTreadMat->setDiffuse(rgba);
    _rTreadMat->setDiffuse(rgba);
}

void TankSceneObject::setTeam(TeamColor team) {
    _bodyMat->clearTextures();
    std::string texname = Team::getImagePrefix(_team) + "tank";
    _bodyMat->setTexture(texname);
}
