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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wno-implicit-function-declaration -Wno-int-conversion -Wno-pointer-sign -Wno-incompatible-pointer-types -Wno-unused-result -Wno-shift-negative-value -fno-strict-aliasing")

# All sources in src/
file(GLOB GAME_SOURCES "src/*.c")

# Vita platform sources
set(VITA_SOURCES
    vita/i_video_vita.c
    vita/i_sound_vita.c
    vita/i_system_vita.c
    vita/i_net_vita.c
)

add_executable(${PROJECT_NAME}
    ${GAME_SOURCES}
    ${VITA_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    vita
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VITA=1
    NORMALUNIX=1
    LINUX=1
)

target_link_libraries(${PROJECT_NAME}
    SDL2_mixer
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
