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

# First, list everything that exists
file(GLOB ALL_DG_FILES "${DG_DIR}/*.c")
message(STATUS "=== ALL .c files in doomgeneric ===")
foreach(f ${ALL_DG_FILES})
    get_filename_component(name ${f} NAME)
    message(STATUS "  ${name}")
endforeach()

# Grab everything, then remove what we DON'T want
file(GLOB DG_SOURCES "${DG_DIR}/*.c")

# Remove by filename pattern - anything platform-specific
set(EXCLUDE_FILES
    "doomgeneric_allegro.c"
    "doomgeneric_sdl.c"
    "doomgeneric_sdl2.c"
    "doomgeneric_win.c"
    "doomgeneric_xlib.c"
    "doomgeneric_soso.c"
    "doomgeneric_txt.c"
    "doomgeneric_ascii.c"
    "doomgeneric_null.c"
    "i_main.c"
    "i_allegromusic.c"
    "i_allegrosound.c"
    "i_sdlmusic.c"
    "i_sdlsound.c"
    "i_oplmusic.c"
    "i_pcsound.c"
)

foreach(exc ${EXCLUDE_FILES})
    set(FULL_PATH "${DG_DIR}/${exc}")
    if(EXISTS "${FULL_PATH}")
        list(REMOVE_ITEM DG_SOURCES "${FULL_PATH}")
        message(STATUS "Excluded: ${exc}")
    endif()
endforeach()

# Safety: also remove anything with allegro or sdl in the name
foreach(src ${DG_SOURCES})
    get_filename_component(name ${src} NAME)
    string(TOLOWER ${name} name_lower)
    if(name_lower MATCHES "allegro" OR name_lower MATCHES "sdl" OR name_lower MATCHES "xlib")
        list(REMOVE_ITEM DG_SOURCES ${src})
        message(STATUS "Excluded (pattern): ${name}")
    endif()
endforeach()

# Also remove any doomgeneric_*.c that isn't our vita or the core
foreach(src ${DG_SOURCES})
    get_filename_component(name ${src} NAME)
    if(name MATCHES "^doomgeneric_" AND NOT name STREQUAL "doomgeneric.c" AND NOT name STREQUAL "doomgeneric_vita.c")
        list(REMOVE_ITEM DG_SOURCES ${src})
        message(STATUS "Excluded (platform): ${name}")
    endif()
endforeach()

# Print final list
message(STATUS "=== FINAL source list ===")
list(LENGTH DG_SOURCES num_src)
message(STATUS "Total: ${num_src} files")
foreach(src ${DG_SOURCES})
    get_filename_component(name ${src} NAME)
    message(STATUS "  COMPILE: ${name}")
endforeach()

# Verify vita file exists
if(NOT EXISTS "${DG_DIR}/doomgeneric_vita.c")
    message(FATAL_ERROR "doomgeneric_vita.c NOT FOUND!")
endif()

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
