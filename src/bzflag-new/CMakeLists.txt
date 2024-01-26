#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#       2020, 2021, 2022, 2023 — Vladimír Vondruš <mosra@centrum.cz>
#
#   This is free and unencumbered software released into the public domain.
#
#   Anyone is free to copy, modify, publish, use, compile, sell, or distribute
#   this software, either in source code form or as a compiled binary, for any
#   purpose, commercial or non-commercial, and by any means.
#
#   In jurisdictions that recognize copyright laws, the author or authors of
#   this software dedicate any and all copyright interest in the software to
#   the public domain. We make this dedication for the benefit of the public
#   at large and to the detriment of our heirs and successors. We intend this
#   dedication to be an overt act of relinquishment in perpetuity of all
#   present and future rights to this software under copyright law.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

cmake_minimum_required(VERSION 3.5)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

#corrade_add_resource(TexturedQuad_RESOURCES resources.conf)

add_executable(bzflag-new WIN32 bzflag-new.cpp ${TexturedQuad_RESOURCES}
ActionBinding.cxx
    ActionBinding.h
    AutoPilot.cxx
    AutoPilot.h
    BackgroundRenderer.cxx
    BackgroundRenderer.h
    BaseLocalPlayer.cxx
    BaseLocalPlayer.h
    CacheMenu.cxx
    CacheMenu.h
    callbacks.cxx
    callbacks.h
    clientCommands.cxx
    clientConfig.cxx
    clientConfig.h
    commands.h
    CommandsImplementation.cxx
    ComposeDefaultKey.cxx
    ComposeDefaultKey.h
    ControlPanel.cxx
    ControlPanel.h
    daylight.cxx
    daylight.h
    defaultBZDB.cxx
    defaultBZDB.h
    DisplayMenu.cxx
    DisplayMenu.h
    Downloads.cxx
    Downloads.h
    EffectsMenu.cxx
    EffectsMenu.h
    effectsRenderer.cxx
    effectsRenderer.h
    EntryZone.cxx
    EntryZone.h
    FlashClock.cxx
    FlashClock.h
    ForceFeedback.cxx
    ForceFeedback.h
    FormatMenu.cxx
    FormatMenu.h
    GuidedMissleStrategy.cxx
    GuidedMissleStrategy.h
    GUIOptionsMenu.cxx
    GUIOptionsMenu.h
    HelpMenu.cxx
    HelpMenu.h
    HUDDialog.cxx
    HUDDialog.h
    HUDDialogStack.cxx
    HUDDialogStack.h
    HUDRenderer.cxx
    HUDRenderer.h
    HUDuiControl.cxx
    HUDuiControl.h
    HUDui.cxx
    HUDuiDefaultKey.cxx
    HUDuiDefaultKey.h
    HUDui.h
    HUDuiLabel.cxx
    HUDuiLabel.h
    HUDuiList.cxx
    HUDuiList.h
    HUDuiTextureLabel.cxx
    HUDuiTextureLabel.h
    HUDuiTypeIn.cxx
    HUDuiTypeIn.h
    InputMenu.cxx
    InputMenu.h
    JoinMenu.cxx
    JoinMenu.h
    KeyboardMapMenu.cxx
    KeyboardMapMenu.h
    LocalCommand.cxx
    LocalCommand.h
    LocalPlayer.cxx
    LocalPlayer.h
    MainMenu.cxx
    MainMenu.h
    MainWindow.cxx
    MainWindow.h
    MenuDefaultKey.cxx
    MenuDefaultKey.h
    motd.cxx
    motd.h
    NewVersionMenu.cxx
    NewVersionMenu.h
    OptionsMenu.cxx
    OptionsMenu.h
    Plan.cxx
    Plan.h
    Player.cxx
    Player.h
    playing.cxx
    playing.h
    QuickKeysMenu.cxx
    QuickKeysMenu.h
    QuitMenu.cxx
    QuitMenu.h
    RadarRenderer.cxx
    RadarRenderer.h
    Region.cxx
    Region.h
    RegionPriorityQueue.cxx
    RegionPriorityQueue.h
    RemotePlayer.cxx
    RemotePlayer.h
    Roaming.cxx
    Roaming.h
    RobotPlayer.cxx
    RobotPlayer.h
    RoofTops.cxx
    RoofTops.h
    Roster.cxx
    Roster.h
    SaveWorldMenu.cxx
    SaveWorldMenu.h
    SceneBuilder.cxx
    SceneBuilder.h
    SceneRenderer.cxx
    ScoreboardRenderer.cxx
    ScoreboardRenderer.h
    SegmentedShotStrategy.cxx
    SegmentedShotStrategy.h
    ServerCommandKey.cxx
    ServerCommandKey.h
    ServerLink.cxx
    ServerLink.h
    ServerListFilter.cxx
    ServerListFilter.h
    ServerListFilterHelpMenu.cxx
    ServerListFilterHelpMenu.h
    ServerListFilterMenu.cxx
    ServerListFilterMenu.h
    ServerMenu.cxx
    ServerMenu.h
    ServerStartMenu.cxx
    ServerStartMenu.h
    ShockWaveStrategy.cxx
    ShockWaveStrategy.h
    ShotPath.cxx
    ShotPath.h
    ShotPathSegment.cxx
    ShotPathSegment.h
    ShotStatistics.cxx
    ShotStatistics.h
    ShotStats.cxx
    ShotStatsDefaultKey.cxx
    ShotStatsDefaultKey.h
    ShotStats.h
    ShotStrategy.cxx
    ShotStrategy.h
    SilenceDefaultKey.cxx
    SilenceDefaultKey.h
    sound.cxx
    sound.h
    stars.cxx
    stars.h
    TargetingUtils.cxx
    TargetingUtils.h
    TrackMarks.cxx
    TrackMarks.h
    Weapon.cxx
    Weapon.h
    WeatherRenderer.cxx
    WeatherRenderer.h
    WorldBuilder.cxx
    WorldBuilder.h
    World.cxx
    World.h
    WorldPlayer.cxx
    WorldPlayer.h
    BZChatConsole.cpp
    ImGuiANSITextRenderer.cpp
    WorldRenderer.cpp
    WorldPrimitiveGenerator.cpp
    Drawables.cpp
    BZScoreboard.cpp)
target_link_libraries(bzflag-new PRIVATE
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
    MagnumIntegration::ImGui
    ${ZLIB_LIBRARIES}
    Threads::Threads
    OpenGL::GL
    OpenGL::GLU
    GLEW::GLEW
    bzcommon
    bz3D
    bznet
    bzplatform
    bzgame
    bzobstacle
    bzscene
    bzmediafile
    bzdate
    bzgeometry
    bzogl)
target_compile_options(bzflag-new PRIVATE -w)
add_dependencies(bzflag-new Magnum::AnyImageImporter Magnum::AnySceneImporter Magnum::ObjImporter)

install(TARGETS bzflag-new DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})

# Make the executable a default target to build & run in Visual Studio
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT bzflag-new)
