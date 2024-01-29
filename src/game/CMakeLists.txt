add_library(bzgame
    BzMaterial.cxx
    CacheManager.cxx
    CollisionManager.cxx
    CommandsStandard.cxx
    DirectoryNames.cxx
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
    ServerList.cxx
    StartupInfo.cxx
    TextureMatrix.cxx
    MagnumBZMaterial.cpp
)

target_link_libraries(bzgame bzcommon Magnum::GL)
