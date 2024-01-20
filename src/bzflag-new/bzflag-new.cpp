/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023 — Vladimír Vondruš <mosra@centrum.cz>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "Magnum/GL/GL.h"
#include "Magnum/SceneGraph/SceneGraph.h"
#include "Magnum/Trade/MaterialData.h"
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/DebugStl.h>

#include <Magnum/ImageView.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/Trade/PhongMaterialData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/TextureData.h>

using namespace Magnum;
using namespace Magnum::Math::Literals;

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

class BZFlagNew: public Platform::Application {
    public:
        explicit BZFlagNew(const Arguments& arguments);

    private:
        void drawEvent() override;
        void viewportEvent(ViewportEvent& e) override;
        void mousePressEvent(MouseEvent& e) override;
        void mouseReleaseEvent(MouseEvent& e) override;
        void mouseMoveEvent(MouseMoveEvent& e) override;
        void mouseScrollEvent(MouseScrollEvent& e) override;

        Vector3 positionOnSphere(const Vector2i& position) const;

        Shaders::PhongGL _coloredShader;
        Shaders::PhongGL _texturedShader{Shaders::PhongGL::Configuration().setFlags(Shaders::PhongGL::Flag::DiffuseTexture)};
        Containers::Array<Containers::Optional<GL::Mesh>> _meshes;
        Containers::Array<Containers::Optional<GL::Texture2D>> _textures;

        Scene3D _scene;
        Object3D _manipulator, _cameraObject;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        Vector3 _previousPosition;
};

BZFlagNew::BZFlagNew(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("BZFlag Magnum Test")
        .setWindowFlags(Configuration::WindowFlag::Resizable)}
{
    using namespace Math::Literals;

    Utility::Arguments args;
    args.addArgument("file").setHelp("file", "file to load")
        .addOption("importer", "AnySceneImporter").setHelp("importer", "importer plugin to use")
        .addSkippedPrefix("magnum", "engine-specific options")
        .setGlobalHelp("Displays a 3D scene file provided on command line.")
        .parse(arguments.argc, arguments.argv);
    
    _cameraObject
        .setParent(&_scene)
        .translate(Vector3::zAxis(5.0f));
    
    (*(_camera = new SceneGraph::Camera3D{_cameraObject}))
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.01f, 1000.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());
    
    _manipulator.setParent(&_scene);


    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    _coloredShader
        .setAmbientColor(0x111111_rgbf)
        .setSpecularColor(0xffffff_rgbf)
        .setShininess(80.0f);
    _texturedShader
        .setAmbientColor(0x111111_rgbf)
        .setSpecularColor(0x111111_rgbf)
        .setShininess(80.0f);

    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate(args.value("importer"));
    if (!importer || !importer->openFile(args.value("file")))
        std::exit(1);

    _textures = Containers::Array<Containers::Optional<GL::Texture2D>>{importer->textureCount()};
    for (UnsignedInt i = 0; i != importer->textureCount(); ++i) {
        Containers::Optional<Trade::TextureData> textureData = importer->texture(i);
        if (!textureData || textureData->type() != Trade::TextureType::Texture2D) {
            Warning{} << "Cannot load texture" << i << importer->textureName(i);
            continue;
        }

        Containers::Optional<Trade::ImageData2D> imageData = importer->image2D(textureData->image());
        if (!imageData || !imageData->isCompressed()) {
            Warning{} << "Cannot load image" << textureData->image() << importer->image2DName(textureData->image());
            continue;
        }

        (*(_textures[i] = GL::Texture2D{}))
            .setMagnificationFilter(textureData->magnificationFilter())
            .setMinificationFilter(textureData->minificationFilter(), textureData->mipmapFilter())
            .setWrapping(textureData->wrapping().xy())
            .setStorage(Math::log2(imageData->size().max()) + 1, GL::textureFormat(imageData->format()), imageData->size())
            .setSubImage(0, {}, *imageData)
            .generateMipmap();
        
        Containers::Array<Containers::Optional<Trade::PhongMaterialData>> materials{importer->materialCount()};
        for (UnsignedInt i = 0; i != importer->materialCount(); ++i) {
            Containers::Optional<Trade::MaterialData> materialData;
            if (!(materialData = importer->material(i))) {
                Warning{} << "Cannot load material" << i << importer->materialName(i);
                continue;
            }
            materials[i] = std::move(*materialData).as<Trade::PhongMaterialData>();
        }

        _meshes = Containers::Array<Containers::Optional<GL::Mesh>>{importer->meshCount()};
        for (UnsignedInt i = 0; i != importer->meshCount(); ++i) {
            Containers::Optional<Trade::MeshData> meshData;
            if (!(meshData = importer->mesh(i))) {
                Warning{} << "Cannot load mesh" << i << importer->meshName(i);
                continue;
            }
            MeshTools::CompileFlags flags;
            if (meshData->hasAttribute(Trade::MeshAttribute::Normal))
                flags |= MeshTools::CompileFlag::GenerateFlatNormals;
            _meshes[i] = MeshTools::compile(*meshData, flags);
        }

        
}

void BZFlagNew::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    _shader
        .setLightPositions({{1.4f, 1.0f, 0.75f, 0.0f}})
        .setDiffuseColor(_color)
        .setAmbientColor(Color3::fromHsv({_color.hue(), 1.0f, 0.3f}))
        .setTransformationMatrix(_transformation)
        .setNormalMatrix(_transformation.normalMatrix())
        .setProjectionMatrix(_projection)
        .draw(_mesh);

    swapBuffers();
}

void BZFlagNew::mousePressEvent(MouseEvent& e) {
    if (e.button() != MouseEvent::Button::Left) return;
    e.setAccepted();
}

void BZFlagNew::mouseReleaseEvent(MouseEvent& e) {
    _color = Color3::fromHsv({_color.hue() + 50.0_degf, 1.0f, 1.0f});

    e.setAccepted();
    redraw();
}

void BZFlagNew::mouseMoveEvent(MouseMoveEvent &e) {
    if (!(e.buttons() & MouseMoveEvent::Button::Left)) return;

    Vector2 delta = 3.0f * Vector2{e.relativePosition()}/Vector2{windowSize()};
    _transformation =
        Matrix4::rotationX(Rad{delta.y()}) * _transformation * Matrix4::rotationY(Rad{delta.x()});
    e.setAccepted();
    redraw();
}

MAGNUM_APPLICATION_MAIN(BZFlagNew)