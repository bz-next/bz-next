#ifndef DEPTHMAPSHADER_H
#define DEPTHMAPSHADER_H

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Attribute.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/GL/Texture.h>

class DepthMapShader : public Magnum::GL::AbstractShaderProgram {
    public:
    typedef Magnum::GL::Attribute<0, Magnum::Vector3> Position;

    explicit DepthMapShader();

    DepthMapShader& setTransformationMatrix(const Magnum::Matrix4& matrix) {
        setUniform(_transformationMatUniform, matrix);
        return *this;
    }

    DepthMapShader& setProjectionMatrix(const Magnum::Matrix4& matrix) {
        setUniform(_projectionMatUniform, matrix);
        return *this;
    }

    private:
    Magnum::Int _transformationMatUniform;
    Magnum::Int _projectionMatUniform;
};

#endif
