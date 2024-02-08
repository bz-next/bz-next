set(BZCOMMON_SOURCES
    AccessList.cxx
    AutoCompleter.cxx
    Bundle.cxx
    BundleMgr.cxx
    BZDBCache.cxx
    BZDBLocal.cxx
    bzfio.cxx
    bzglob.cxx
    bz_Locale.cxx
    BzPNG.cxx
    bzsignal.cxx
    CMakeLists.txt
    CommandManager.cxx
    ConfigFileManager.cxx
    Country.cxx
    ErrorHandler.cxx
    FileManager.cxx
    Flag.cxx
    GameTime.cxx
    global.cxx
    KeyManager.cxx
    Language.cxx
    md5.cxx
    messages.cxx
    OSFile.cxx
    ParseColor.cxx
    PlayerState.cxx
    ShotUpdate.cxx
    StateDatabase.cxx
    Team.cxx
    TextChunkManager.cxx
    TextUtils.cxx
    TimeKeeper.cxx
    VotingBooth.cxx
    WordFilter.cxx
)

if (NOT CORRADE_TARGET_EMSCRIPTEN)
    list(APPEND BZCOMMON_SOURCES cURLManager.cxx)
endif()

add_library(bzcommon STATIC ${BZCOMMON_SOURCES})

target_link_libraries(bzcommon
    ${CURL_LIBRARIES}
)
