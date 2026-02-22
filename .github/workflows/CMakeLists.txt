cmake_minimum_required(VERSION 3.2)

# ── Toolchain Vita ────────────────────────────────────────────
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake"
        CACHE PATH "Vita toolchain file")
  else()
    message(FATAL_ERROR
      "Definisci la variabile d'ambiente VITASDK (es. /usr/local/vitasdk)")
  endif()
endif()

project(chexquest-vita C)

include("${VITASDK}/share/vita.cmake" REQUIRED)

# ── Metadati VPK ──────────────────────────────────────────────
set(VITA_APP_NAME  "Chex Quest")
set(VITA_TITLEID   "CHEX00001")
set(VITA_VERSION   "01.00")
