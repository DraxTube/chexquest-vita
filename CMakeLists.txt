cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(ChexQuestVita C)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Chex Quest")
set(VITA_TITLEID  "CHEX00001")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wno-unused-variable -Wno-unused-function -Wno-missing-braces -fno-strict-aliasing")

add_definitions(-DVITA -DNORMALUNIX -DLINUX -DSNDSERV -DHAVE_LIBSDL2)

set(SOURCES
    src/main.c
    src/d_main.c
    src/i_video_vita.c
    src/i_sound_vita.c
    src/i_input_vita.c
    src/i_system_vita.c
    src/i_net_vita.c
    src/w_wad.c
    src/r_main.c
    src/r_draw.c
    src/r_plane.c
    src/r_things.c
    src/r_bsp.c
    src/r_segs.c
    src/r_sky.c
    src/r_data.c
    src/p_setup.c
    src/p_map.c
    src/p_mobj.c
    src/p_enemy.c
    src/p_tick.c
    src/p_user.c
    src/p_inter.c
    src/p_spec.c
    src/p_sight.c
    src/p_maputl.c
    src/p_floor.c
    src/p_doors.c
    src/p_ceilng.c
    src/p_plats.c
    src/p_lights.c
    src/p_switch.c
    src/p_telept.c
    src/p_saveg.c
    src/g_game.c
    src/s_sound.c
    src/hu_stuff.c
    src/st_stuff.c
    src/am_map.c
    src/m_menu.c
    src/m_misc.c
    src/m_random.c
    src/m_fixed.c
    src/m_argv.c
    src/m_bbox.c
    src/m_cheat.c
    src/f_finale.c
    src/f_wipe.c
    src/info.c
    src/sounds.c
    src/tables.c
    src/v_video.c
    src/z_zone.c
    src/d_items.c
    src/d_net.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME}
    SDL2_mixer
    SDL2
    vorbisfile
    vorbis
    ogg
    mikmod
    mpg123
    FLAC
    vita2d
    SceDisplay_stub
    SceCtrl_stub
    SceTouch_stub
    SceAudio_stub
    SceAudioIn_stub
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
    SceMotion_stub
    ScePower_stub
    SceRtc_stub
    SceHid_stub
    pthread
    z
    m
    c
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
