add_executable(bzadmin
    BZAdminClient.cxx
    BZAdminClient.h
    bzadmin.cxx
    BZAdminUI.cxx
    BZAdminUI.h
    CMakeLists.txt
    colors.h
    curses_wrapper.h
    OptionParser.cxx
    OptionParser.h
    PlayerInfo.h
    ServerLink.cxx
    ServerLink.h
    StdBothUI.cxx
    StdBothUI.h
    StdInUI.cxx
    StdInUI.h
    StdOutUI.cxx
    StdOutUI.h
    UIMap.cxx
    UIMap.h
)

if(CURSES_FOUND)
    target_sources(bzadmin PRIVATE
        CursesMenu.cxx
        CursesMenu.h
        CursesUI.cxx
        CursesUI.h
    )
endif(CURSES_FOUND)

target_link_libraries(bzadmin
    ${CURSES_LIBRARIES}
    ${CURL_LIBRARIES}
    bzcommon
    bznet
    bzgame
    bzdate
)

