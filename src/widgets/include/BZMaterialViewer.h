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

class BZMaterialViewer {
    public:
    BZMaterialViewer();
    void draw(const char* title, bool* p_open);

    
    private:
    int itemCurrent;
    Magnum::GL::Texture2D renderTex;

    Magnum::GL::Texture2D tex;
    Magnum::GL::Mesh mesh;
    Magnum::Shaders::PhongGL phong;
    Magnum::Shaders::PhongGL phongUntex;
    

    void renderPreview();
    
    const Magnum::Math::Vector2<int> bufsize{256, 256};
};

#endif
