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
set(VITA_TITLEID  "CQST00001")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -w -fno-strict-aliasing -DNORMALUNIX -DLINUX -DVITA")

set(DG_DIR "${CMAKE_SOURCE_DIR}/doomgeneric/doomgeneric")

file(GLOB DG_SOURCES "${DG_DIR}/*.c")

set(EXCLUDE_FILES
    "doomgeneric_allegro.c"
    "doomgeneric_sdl.c"
    "doomgeneric_sdl2.c"
    "doomgeneric_win.c"
    "doomgeneric_xlib.c"
    "doomgeneric_soso.c"
    "doomgeneric_sosox.c"
    "doomgeneric_txt.c"
    "doomgeneric_ascii.c"
    "doomgeneric_null.c"
    "doomgeneric_emscripten.c"
    "doomgeneric_linuxvt.c"
    "i_allegromusic.c"
    "i_allegrosound.c"
    "i_sdlmusic.c"
    "i_sdlsound.c"
    "i_oplmusic.c"
    "i_pcsound.c"
    "i_sound.c"
    "i_video.c"
    "i_input.c"
    "i_joystick.c"
    "i_timer.c"
    "i_system.c"
    "i_cdmus.c"
    "i_endoom.c"
    "i_scale.c"
    "i_main.c"
    "icon.c"
    "gusconf.c"
)

foreach(exc ${EXCLUDE_FILES})
    set(FULL_PATH "${DG_DIR}/${exc}")
    if(EXISTS "${FULL_PATH}")
        list(REMOVE_ITEM DG_SOURCES "${FULL_PATH}")
    endif()
endforeach()

foreach(src ${DG_SOURCES})
    get_filename_component(name ${src} NAME)
    if(name MATCHES "^doomgeneric_"
       AND NOT name STREQUAL "doomgeneric.c"
       AND NOT name STREQUAL "doomgeneric_vita.c")
        list(REMOVE_ITEM DG_SOURCES ${src})
    endif()
endforeach()

# Assicurati che doomgeneric_vita.c sia incluso
set(VITA_SRC "${DG_DIR}/doomgeneric_vita.c")
if(NOT EXISTS "${VITA_SRC}")
    set(VITA_SRC "${CMAKE_SOURCE_DIR}/doomgeneric_vita.c")
endif()
if(NOT EXISTS "${VITA_SRC}")
    set(VITA_SRC "${CMAKE_SOURCE_DIR}/doomgeneric/doomgeneric_vita.c")
endif()

list(FIND DG_SOURCES "${VITA_SRC}" VITA_IDX)
if(VITA_IDX EQUAL -1)
    if(EXISTS "${VITA_SRC}")
        list(APPEND DG_SOURCES "${VITA_SRC}")
        message(STATUS "Added vita source: ${VITA_SRC}")
    else()
        message(FATAL_ERROR "Cannot find doomgeneric_vita.c!")
    endif()
endif()

add_executable(${PROJECT_NAME} ${DG_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${DG_DIR})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VITA=1
    FEATURE_SOUND=1
    DOOMGENERIC_RESX=320
    DOOMGENERIC_RESY=200
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
    m
    c
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
    VERSION ${VITA_VERSION}
    NAME ${VITA_APP_NAME}
)
