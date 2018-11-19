cmake_minimum_required (VERSION 3.12)
project (fchat)
#add_library(fchat src/fchat.c)
set (fchat_VERSION_MAJOR 1)
set (fchat_VERSION_MINOR 0)

# Set global compiler-specific flags

if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wshadow -fvisibility=hidden -std=c11")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -ggdb")
endif()

include(FindPkgConfig)

pkg_check_modules(GLIB REQUIRED glib-2.0)

pkg_check_modules(GIO REQUIRED gio-2.0)
include_directories(${GIO_INCLUDE_DIRS})
add_definitions(${GIO_CFLAGS_OTHER})
link_directories(${GIO_LIBRARY_DIRS})
list(APPEND EXTRA_LIBRARIES ${GIO_LIBRARIES})

# Libpurple
add_definitions(-DPURPLE_PLUGINS)

pkg_check_modules(PURPLE REQUIRED purple)
include_directories(${PURPLE_INCLUDE_DIRS})
add_definitions(${PURPLE_CFLAGS_OTHER})
link_directories(${PURPLE_LIBRARY_DIRS})
list(APPEND EXTRA_LIBRARIES ${PURPLE_LIBRARIES})

# Store path to libpurple plugins directory for later install() command.
if (UNIX AND NOT APPLE)
    execute_process(COMMAND pkg-config purple --variable=plugindir
        OUTPUT_VARIABLE PURPLE_PLUGIN_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

include_directories(src)
set(SOURCES
        src/actions.c
        src/attention.c
        src/buddy.c
        src/chat.c
        src/connection.c
        src/fchat.c
        src/fchat.h
        src/info.c
        src/process_cmds.c
        src/send_cmds.c
        src/status.c
)

# Primary plugin library.

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${EXTRA_LIBRARIES})

# Install target for Linux (not tested on BSD)

if(UNIX AND NOT APPLE)
    # Default install prefix is /usr/local, change it to /usr
    if(NOT DEFINED CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/usr")
    endif()

    install(TARGETS ${PROJECT_NAME} DESTINATION ${PURPLE_PLUGIN_DIR})
    install(DIRECTORY "share/pixmaps/pidgin/protocols" DESTINATION "share/pixmaps/pidgin")
endif()
