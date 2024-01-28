#ifndef BZMATERIALVIEWER_H
#define BZMATERIALVIEWER_H

#include <Magnum/ImGuiIntegration/Context.hpp>

#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Texture.h>

#include <Magnum/GL/GL.h>
#include <Magnum/GL/Attribute.h>
#include <Magnum/Math/Vector.h>
#include <Magnum/Math/Color.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/Shaders/VertexColorGL.h>
#include "Magnum/Shaders/Shaders.h"
#include "MagnumTextureManager.h"
#include <Magnum/Shaders/PhongGL.h>

class MatViewerShader: public Magnum::GL::AbstractShaderProgram {
    public:
        typedef Magnum::GL::Attribute<0, Magnum::Math::Vector2<float>> Position;
        typedef Magnum::GL::Attribute<1, Magnum::Math::Vector2<float>> TextureCoordinates;

        explicit MatViewerShader();

        MatViewerShader& setColor(const Magnum::Color3& color) {
            setUniform(_colorUniform, color);
            return *this;
        }

        MatViewerShader& bindTexture(Magnum::GL::Texture2D& texture) {
            texture.bind(TextureUnit);
            return *this;
        }

    private:
        enum: Magnum::Int { TextureUnit = 0 };

        Magnum::Int _colorUniform;
};

class BZMaterialViewer {
    public:
    BZMaterialViewer();
    void draw(const char* title, bool* p_open);

    
    private:
    int itemCurrent;
    Magnum::GL::Texture2D renderTex;

    Magnum::GL::Texture2D tex;
    Magnum::GL::Mesh mesh;
    MatViewerShader shader;
    Magnum::Shaders::PhongGL phong;
    

    void renderPreview();

    const Magnum::Math::Vector2<int> bufsize{256, 256};
};

#endif
