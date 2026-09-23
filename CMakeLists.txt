# ===================================================================
# Tibia 860 - Extended Client DLL
# Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)
# ===================================================================

cmake_minimum_required(VERSION 3.15)
project(ExtendedDLL C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enforce 32-bit (x86) architecture required by the Tibia 8.60 client
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(WARNING "The Tibia 8.60 client is a 32-bit (x86) executable. Please ensure your compiler target is set to Win32/x86.")
endif()

# Preprocessor Definitions
add_definitions(
    -DWIN32
    -D_WINDOWS
    -D_USRDLL
    -DEXTENDED_DLL_EXPORTS
    -D__INCLUDE_860_VERSION__
    -D__CONFIG__
    -D__EXTENDED_FILE__
    -D__ALPHA_SPRITES__
    -D__MAGIC_EFFECTS_U16__
    -D__PLAYER_HEALTH_U32__
    -D__PLAYER_MANA_U32__
    -D__PLAYER_SKILLS_U16__
    -D_CRT_SECURE_NO_WARNINGS
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(SOURCES
    src/dllmain.cpp
    src/ddraw_proxy.cpp
    src/hook.cpp
    src/sprites.cpp
    src/ext_engines.cpp
    src/creature_manager.cpp
    src/network_protocol.cpp
    src/ui_market.cpp
    src/market_window.cpp
    src/outfit_window.cpp
    src/timer.cpp
    src/dat_reader.cpp
)

set(HEADERS
    include/main.h
    include/features.h
    include/config.h
    include/ddraw_proxy.h
    include/hook.h
    include/sprites.h
    include/ext_engines.h
    include/creature_manager.h
    include/network_protocol.h
    include/ui_market.h
    include/market_window.h
    include/outfit_window.h
    include/timer.h
    include/dat_reader.h
)

add_library(ddraw SHARED ${SOURCES} ${HEADERS} ddraw.def)

set_target_properties(ddraw PROPERTIES
    OUTPUT_NAME "ddraw"
    PREFIX ""
    SUFFIX ".dll"
)

# Required System Libraries
target_link_libraries(ddraw
    opengl32
    glu32
    d3d9
    winmm
    user32
    gdi32
    kernel32
)

if(MSVC)
    target_compile_options(ddraw PRIVATE /W3 /O2 /Oi /Oy /Gy /GL /GS-)
    set_target_properties(ddraw PROPERTIES LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/ddraw.def\" /SAFESEH:NO")
endif()
