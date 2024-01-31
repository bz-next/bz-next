# These libraries are needed for everything
add_subdirectory(common)
add_subdirectory(date)
add_subdirectory(game)
add_subdirectory(net)

# obstacle is used for both the client and the server
if(ENABLE_SERVER OR CLIENT_INCLUDED)
    add_subdirectory(obstacle)
endif(ENABLE_SERVER OR CLIENT_INCLUDED)

if(ENABLE_SERVER)
    add_subdirectory(bzfs)
endif(ENABLE_SERVER)

# The client makes use of several of our libraries
if(CLIENT_INCLUDED)
    add_subdirectory(3D)
    add_subdirectory(geometry)
    add_subdirectory(mediafile)
    add_subdirectory(ogl)
    add_subdirectory(platform)
    add_subdirectory(scene)
    #add_subdirectory(bzflag)
endif(CLIENT_INCLUDED)

add_subdirectory(bzflag-new)

if(ENABLE_BZADMIN)
    add_subdirectory(bzadmin)
endif(ENABLE_BZADMIN)
