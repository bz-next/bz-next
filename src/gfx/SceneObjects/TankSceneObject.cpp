#include "TankSceneObject.h"

#include "TextureMatrix.h"
#include "common.h"

#include "Team.h"
#include <string>

using namespace Magnum;

void TankSceneObject::setPosition(const float pos[3], const float angleRad) {
    Object3D::resetTransformation();
    //Object3D::scale({10.0f, 10.0f, 10.0f});
    Object3D::rotateZ(Math::Rad<float>(angleRad));
    Object3D::translate({pos[0], pos[1], pos[2]});
    /*for (int i = LeftWheel0; i <= LeftWheel3; ++i) {
        _parts[i]->rotateY(Math::Rad<float>(_leftTreadOffset));
    }
    for (int i = RightWheel0; i <= RightWheel3; ++i) {
        _parts[i]->rotateY(Math::Rad<float>(_rightTreadOffset));
    }*/
    
}

void TankSceneObject::addTreadOffsets(float left, float right) {
    // These all need magic multiplication factors so that they look
    // roughly correct. Do that later... Just pick factors that look OK.
    _leftTreadOffset += 0.1f*left;
    _rightTreadOffset += 0.1f*right;
    if (_lTreadMat) {
        auto mat = TEXMATRIXMGR.getMatrix(_lTreadMat->getTextureMatrix(0));
        mat->setStaticShift(-_leftTreadOffset, 0.0f);
        mat->finalize();
    }
    if (_rTreadMat) {
        auto mat = TEXMATRIXMGR.getMatrix(_rTreadMat->getTextureMatrix(0));
        mat->setStaticShift(-_rightTreadOffset, 0.0f);
        mat->finalize();
    }
    if (_lWheelMat) {
        auto mat = TEXMATRIXMGR.getMatrix(_lWheelMat->getTextureMatrix(0));
        if (mat) {
            mat->setStaticSpin(-500.0f*_leftTreadOffset);
            mat->setStaticCenter(0.5f, 0.5f);
            mat->finalize();
        } else std::cout << "here\n";
    }
    if (_rWheelMat) {
        auto mat = TEXMATRIXMGR.getMatrix(_rWheelMat->getTextureMatrix(0));
        if (mat) {
            mat->setStaticSpin(-500.0f*_rightTreadOffset);
            mat->setStaticCenter(0.5f, 0.5f);
            mat->finalize();
        } else std::cout << "here\n";
    }
}

void TankSceneObject::setExplodeFraction(float t) {
    if (t == 0.0f) {
        if (_isExploding)
            for (int i = 0; i < LastTankPart; ++i) {
                _parts[i]->resetTransformation();
            }
        _isExploding = false;
        return;
    }
    _explodeFraction = t;
    for (int i = 0; i < LastTankPart; ++i) {
        _parts[i]->resetTransformation();
        _parts[i]->rotate(Math::Rad<float>(t*_spin[i][3]), {_spin[i][0], _spin[i][1], _spin[i][2]});
        _parts[i]->translate({t*_vel[i][0], t*_vel[i][1], t*_vel[i][2]});
    }
    _isExploding = true;
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
    if (_bodyMat) _bodyMat->setDiffuseAlpha(rgba[3]);
    if (_lTreadMat) _lTreadMat->setDiffuseAlpha(rgba[3]);
    if (_rTreadMat) _rTreadMat->setDiffuseAlpha(rgba[3]);
    if (_barrelMat) _barrelMat->setDiffuseAlpha(rgba[3]);
    if (_lWheelMat) _lWheelMat->setDiffuseAlpha(rgba[3]);
    if (_rWheelMat) _rWheelMat->setDiffuseAlpha(rgba[3]);
}

void TankSceneObject::setTeam(TeamColor team) {
    if (_bodyMat) {
        _bodyMat->clearTextures();
        std::string texname = Team::getImagePrefix(_team) + "tank";
        _bodyMat->setTexture(texname);
    }
    if (_lWheelMat) {
        _lWheelMat->clearTextures();
        std::string texname = Team::getImagePrefix(_team) + "tank";
        _lWheelMat->setTexture(texname);
        _lWheelMat->setTextureMatrix(TEXMATRIXMGR.findMatrix(_lWheelMat->getName()));
    }
    if (_rWheelMat) {
        _rWheelMat->clearTextures();
        std::string texname = Team::getImagePrefix(_team) + "tank";
        _rWheelMat->setTexture(texname);
        _rWheelMat->setTextureMatrix(TEXMATRIXMGR.findMatrix(_rWheelMat->getName()));
    }
    // Quick hack to render invisible "players" invisible
    if (!Team::isPlayableTeam(team)) {
        float rgba[4] = {1.0f, 1.0f, 1.0f, 0.0f};
        setColor(rgba);
    }
}
