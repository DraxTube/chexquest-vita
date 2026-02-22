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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -Wall -Wno-unused-variable -Wno-unused-function -Wno-missing-braces -Wno-implicit-function-declaration -fno-strict-aliasing -DVITA")

# Chocolate Doom source directory
set(CHOCO_SRC "${CMAKE_SOURCE_DIR}/chocolate-doom/src")
set(CHOCO_DOOM "${CHOCO_SRC}/doom")

# Collect Chocolate Doom source files
file(GLOB CHOCO_CORE_SRC "${CHOCO_SRC}/*.c")
file(GLOB CHOCO_DOOM_SRC "${CHOCO_DOOM}/*.c")

# Vita-specific sources
set(VITA_SRC
    ${CMAKE_SOURCE_DIR}/vita/i_video_vita.c
    ${CMAKE_SOURCE_DIR}/vita/i_input_vita.c
    ${CMAKE_SOURCE_DIR}/vita/i_sound_vita.c
    ${CMAKE_SOURCE_DIR}/vita/i_system_vita.c
)

# Remove platform-specific files that we're replacing
list(FILTER CHOCO_CORE_SRC EXCLUDE REGEX ".*i_sdlvideo\\.c$")
list(FILTER CHOCO_CORE_SRC EXCLUDE REGEX ".*i_sdlsound\\.c$")
list(FILTER CHOCO_CORE_SRC EXCLUDE REGEX ".*i_sdlmusic\\.c$")
list(FILTER CHOCO_CORE_SRC EXCLUDE REGEX ".*i_oplmusic\\.c$")
list(FILTER CHOCO_CORE_SRC EXCLUDE REGEX ".*i_pcsound\\.c$")
list(FILTER CHOCO_CORE_SRC EXCLUDE REGEX ".*i_winmusic\\.c$")

# Combine all sources
set(ALL_SOURCES
    ${CHOCO_CORE_SRC}
    ${CHOCO_DOOM_SRC}
    ${VITA_SRC}
)

add_executable(${PROJECT_NAME} ${ALL_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CHOCO_SRC}
    ${CHOCO_DOOM}
    ${CMAKE_SOURCE_DIR}/vita
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VITA
    PACKAGE_NAME="ChexQuest"
    PACKAGE_VERSION="1.0"
    PACKAGE_STRING="ChexQuest 1.0"
    PACKAGE_TARNAME="chexquest"
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
    FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
