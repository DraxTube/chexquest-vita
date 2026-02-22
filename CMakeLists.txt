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

# Collect all C files
file(GLOB DG_SOURCES "${DG_DIR}/*.c")

# Remove ALL other platform files - keep ONLY our vita one
# This is the fix: use a whitelist approach instead of blacklist
set(PLATFORM_FILES_TO_REMOVE)
foreach(src ${DG_SOURCES})
    get_filename_component(name ${src} NAME)
    if(name MATCHES "^doomgeneric_.*\\.c$" AND NOT name STREQUAL "doomgeneric_vita.c" AND NOT name STREQUAL "doomgeneric.c")
        list(APPEND PLATFORM_FILES_TO_REMOVE ${src})
    endif()
endforeach()

foreach(src ${PLATFORM_FILES_TO_REMOVE})
    list(REMOVE_ITEM DG_SOURCES ${src})
    get_filename_component(name ${src} NAME)
    message(STATUS "Excluded: ${name}")
endforeach()

# Also remove i_main.c if it has its own main()
foreach(src ${DG_SOURCES})
    get_filename_component(name ${src} NAME)
    if(name STREQUAL "i_main.c")
        list(REMOVE_ITEM DG_SOURCES ${src})
        message(STATUS "Excluded: ${name}")
    endif()
endforeach()

# Print what we ARE compiling
message(STATUS "=== Compiling ===")
foreach(src ${DG_SOURCES})
    get_filename_component(name ${src} NAME)
    message(STATUS "  ${name}")
endforeach()

add_executable(${PROJECT_NAME} ${DG_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${DG_DIR}
    ${CMAKE_SOURCE_DIR}/vita
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VITA=1
    FEATURE_SOUND=0
)

target_link_libraries(${PROJECT_NAME}
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
    SceHid_stub
    pthread
    png
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
