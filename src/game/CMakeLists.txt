set(BZGAME_SOURCES
BzMaterial.cxx
CacheManager.cxx
CollisionManager.cxx
CommandsStandard.cxx
DirectoryNames.cxx
Downloads.cxx
DynamicColor.cxx
Frustum.cxx
Intersect.cxx
LagInfo.cxx
LinkManager.cxx
MeshTransform.cxx
MsgStrings.cxx
NetHandler.cxx
PhysicsDriver.cxx
PlayerInfo.cxx
Ray.cxx
ServerItem.cxx
ServerListCache.cxx
StartupInfo.cxx
TextureMatrix.cxx
MagnumBZMaterial.cpp
)

if (NOT CORRADE_TARGET_EMSCRIPTEN)
    list(APPEND BZGAME_SOURCES ServerList.cxx)
endif()

add_library(bzgame STATIC ${BZGAME_SOURCES})

target_link_libraries(bzgame bzcommon bznet Magnum::GL)
