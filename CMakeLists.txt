cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(ChexQuestVita C)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Chex Quest")
set(VITA_TITLEID  "CHEX00001")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-missing-braces -Wno-implicit-function-declaration -Wno-parentheses -Wno-pointer-sign -fno-strict-aliasing")

# ============================================================
# Source paths
# ============================================================
set(CHOCO_SRC "${CMAKE_SOURCE_DIR}/chocolate-doom/src")
set(CHOCO_DOOM "${CHOCO_SRC}/doom")

# ============================================================
# Chocolate Doom core sources (hand-picked to avoid conflicts)
# ============================================================
set(CHOCO_CORE_SOURCES
    ${CHOCO_SRC}/d_event.c
    ${CHOCO_SRC}/d_iwad.c
    ${CHOCO_SRC}/d_loop.c
    ${CHOCO_SRC}/d_mode.c
    ${CHOCO_SRC}/deh_io.c
    ${CHOCO_SRC}/deh_main.c
    ${CHOCO_SRC}/deh_mapping.c
    ${CHOCO_SRC}/deh_str.c
    ${CHOCO_SRC}/deh_text.c
    ${CHOCO_SRC}/gusconf.c
    ${CHOCO_SRC}/i_cdmus.c
    ${CHOCO_SRC}/i_endoom.c
    ${CHOCO_SRC}/i_joystick.c
    ${CHOCO_SRC}/i_main.c
    ${CHOCO_SRC}/i_scale.c
    ${CHOCO_SRC}/i_sdlmusic.c
    ${CHOCO_SRC}/i_sdlsound.c
    ${CHOCO_SRC}/i_sound.c
    ${CHOCO_SRC}/i_system.c
    ${CHOCO_SRC}/i_timer.c
    ${CHOCO_SRC}/i_video.c
    ${CHOCO_SRC}/i_videohr.c
    ${CHOCO_SRC}/icon.c
    ${CHOCO_SRC}/m_argv.c
    ${CHOCO_SRC}/m_bbox.c
    ${CHOCO_SRC}/m_cheat.c
    ${CHOCO_SRC}/m_config.c
    ${CHOCO_SRC}/m_controls.c
    ${CHOCO_SRC}/m_fixed.c
    ${CHOCO_SRC}/m_misc.c
    ${CHOCO_SRC}/memio.c
    ${CHOCO_SRC}/midifile.c
    ${CHOCO_SRC}/mus2mid.c
    ${CHOCO_SRC}/net_client.c
    ${CHOCO_SRC}/net_common.c
    ${CHOCO_SRC}/net_dedicated.c
    ${CHOCO_SRC}/net_io.c
    ${CHOCO_SRC}/net_loop.c
    ${CHOCO_SRC}/net_packet.c
    ${CHOCO_SRC}/net_query.c
    ${CHOCO_SRC}/net_sdl.c
    ${CHOCO_SRC}/net_server.c
    ${CHOCO_SRC}/net_structrw.c
    ${CHOCO_SRC}/sha1.c
    ${CHOCO_SRC}/tables.c
    ${CHOCO_SRC}/v_video.c
    ${CHOCO_SRC}/w_checksum.c
    ${CHOCO_SRC}/w_file.c
    ${CHOCO_SRC}/w_file_stdc.c
    ${CHOCO_SRC}/w_main.c
    ${CHOCO_SRC}/w_wad.c
    ${CHOCO_SRC}/z_zone.c
)

# ============================================================
# Chocolate Doom - Doom game sources
# ============================================================
file(GLOB CHOCO_DOOM_SOURCES "${CHOCO_DOOM}/*.c")

# ============================================================
# Vita-specific sources
# ============================================================
set(VITA_SOURCES
    ${CMAKE_SOURCE_DIR}/vita/vita_main.c
    ${CMAKE_SOURCE_DIR}/vita/i_video_vita.c
    ${CMAKE_SOURCE_DIR}/vita/i_input_vita.c
    ${CMAKE_SOURCE_DIR}/vita/i_sound_vita.c
    ${CMAKE_SOURCE_DIR}/vita/i_system_vita.c
)

# ============================================================
# Filter out files that might not exist
# ============================================================
set(FILTERED_CORE_SOURCES)
foreach(src ${CHOCO_CORE_SOURCES})
    if(EXISTS "${src}")
        list(APPEND FILTERED_CORE_SOURCES "${src}")
    else()
        message(STATUS "Skipping missing file: ${src}")
    endif()
endforeach()

# Combine
set(ALL_SOURCES
    ${FILTERED_CORE_SOURCES}
    ${CHOCO_DOOM_SOURCES}
    ${VITA_SOURCES}
)

add_executable(${PROJECT_NAME} ${ALL_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CHOCO_SRC}
    ${CHOCO_DOOM}
    ${CMAKE_SOURCE_DIR}/chocolate-doom
    ${CMAKE_SOURCE_DIR}/vita
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VITA=1
    __vita__=1
    HAVE_LIBSDL2=1
    HAVE_LIBSDL2_MIXER=1
    HAVE_LIBSDL2_NET=1
    PACKAGE_NAME="ChexQuestVita"
    PACKAGE_VERSION="1.0.0"
    PACKAGE_STRING="ChexQuestVita_1.0.0"
    PACKAGE_TARNAME="chexquestvita"
    PROGRAM_PREFIX=""
    HAVE_CONFIG_H=1
)

target_link_libraries(${PROJECT_NAME}
    SDL2_mixer
    SDL2_net
    SDL2
    vorbisfile
    vorbis
    ogg
    mikmod
    FLAC
    mpg123
    png
    vita2d
    SceDisplay_stub
    SceCtrl_stub
    SceTouch_stub
    SceAudio_stub
    SceSysmodule_stub
    SceGxm_stub
    SceCommonDialog_stub
    ScePgf_stub
    SceAppMgr_stub
    SceAppUtil_stub
    SceIofilemgr_stub
    SceFios2_stub
    SceKernelDmacMgr_stub
    SceKernelThreadMgr_stub
    ScePower_stub
    SceRtc_stub
    SceNet_stub
    SceNetCtl_stub
    SceHid_stub
    pthread
    z
    m
    c
    stdc++
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
    VERSION ${VITA_VERSION}
    NAME ${VITA_APP_NAME}
    FILE sce_sys/icon0.png sce_sys/icon0.png
    FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
    FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
