cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain")
  else()
    message(FATAL_ERROR "Please define VITASDK")
  endif()
endif()

project(ChexQuestVita C)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Chex Quest")
set(VITA_TITLEID  "CHEX00001")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -w -fno-strict-aliasing -DNORMALUNIX -DLINUX -DVITA")

set(DG_DIR "${CMAKE_SOURCE_DIR}/doomgeneric/doomgeneric")

# Instead of glob + filter, list EXACTLY the files we need.
# This way nothing unexpected ever gets compiled.
set(DG_SOURCES
    ${DG_DIR}/am_map.c
    ${DG_DIR}/d_event.c
    ${DG_DIR}/d_items.c
    ${DG_DIR}/d_iwad.c
    ${DG_DIR}/d_loop.c
    ${DG_DIR}/d_main.c
    ${DG_DIR}/d_mode.c
    ${DG_DIR}/d_net.c
    ${DG_DIR}/doomdef.c
    ${DG_DIR}/doomgeneric.c
    ${DG_DIR}/doomgeneric_vita.c
    ${DG_DIR}/doomstat.c
    ${DG_DIR}/dstrings.c
    ${DG_DIR}/f_finale.c
    ${DG_DIR}/f_wipe.c
    ${DG_DIR}/g_game.c
    ${DG_DIR}/hu_lib.c
    ${DG_DIR}/hu_stuff.c
    ${DG_DIR}/i_sound.c
    ${DG_DIR}/i_system.c
    ${DG_DIR}/i_timer.c
    ${DG_DIR}/i_video.c
    ${DG_DIR}/info.c
    ${DG_DIR}/m_argv.c
    ${DG_DIR}/m_bbox.c
    ${DG_DIR}/m_cheat.c
    ${DG_DIR}/m_config.c
    ${DG_DIR}/m_controls.c
    ${DG_DIR}/m_fixed.c
    ${DG_DIR}/m_menu.c
    ${DG_DIR}/m_misc.c
    ${DG_DIR}/m_random.c
    ${DG_DIR}/memio.c
    ${DG_DIR}/mus2mid.c
    ${DG_DIR}/p_ceilng.c
    ${DG_DIR}/p_doors.c
    ${DG_DIR}/p_enemy.c
    ${DG_DIR}/p_floor.c
    ${DG_DIR}/p_inter.c
    ${DG_DIR}/p_lights.c
    ${DG_DIR}/p_map.c
    ${DG_DIR}/p_maputl.c
    ${DG_DIR}/p_mobj.c
    ${DG_DIR}/p_plats.c
    ${DG_DIR}/p_pspr.c
    ${DG_DIR}/p_saveg.c
    ${DG_DIR}/p_setup.c
    ${DG_DIR}/p_sight.c
    ${DG_DIR}/p_spec.c
    ${DG_DIR}/p_switch.c
    ${DG_DIR}/p_telept.c
    ${DG_DIR}/p_tick.c
    ${DG_DIR}/p_user.c
    ${DG_DIR}/r_bsp.c
    ${DG_DIR}/r_data.c
    ${DG_DIR}/r_draw.c
    ${DG_DIR}/r_main.c
    ${DG_DIR}/r_plane.c
    ${DG_DIR}/r_segs.c
    ${DG_DIR}/r_sky.c
    ${DG_DIR}/r_things.c
    ${DG_DIR}/s_sound.c
    ${DG_DIR}/sha1.c
    ${DG_DIR}/sounds.c
    ${DG_DIR}/st_lib.c
    ${DG_DIR}/st_stuff.c
    ${DG_DIR}/statdump.c
    ${DG_DIR}/tables.c
    ${DG_DIR}/v_video.c
    ${DG_DIR}/w_checksum.c
    ${DG_DIR}/w_file.c
    ${DG_DIR}/w_file_stdc.c
    ${DG_DIR}/w_wad.c
    ${DG_DIR}/wi_stuff.c
    ${DG_DIR}/z_zone.c
)

add_executable(${PROJECT_NAME} ${DG_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${DG_DIR})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VITA=1
    FEATURE_SOUND=0
)

target_link_libraries(${PROJECT_NAME}
    SceDisplay_stub
    SceCtrl_stub
    SceTouch_stub
    SceAudio_stub
    SceSysmodule_stub
    SceGxm_stub
    SceCommonDialog_stub
    SceAppMgr_stub
    SceAppUtil_stub
    SceIofilemgr_stub
    SceKernelDmacMgr_stub
    SceKernelThreadMgr_stub
    ScePower_stub
    SceRtc_stub
    SceHid_stub
    SceSysmem_stub
    pthread
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
