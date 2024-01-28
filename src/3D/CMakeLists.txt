add_library(bz3D
    FontManager.cxx
    ImageFont.cxx
    ImageFont.h
    TextureFont.cxx
    TextureFont.h
    TextureManager.cxx
    MagnumTextureManager.cpp
)

add_dependencies(bz3D Magnum::AnyImageImporter)

target_link_libraries(bz3D PRIVATE
    Corrade::Main
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::Shaders
    Magnum::Trade
    Magnum::Primitives
    Magnum::MeshTools
    Magnum::SceneGraph
    Magnum::DebugTools
    MagnumIntegration::ImGui)