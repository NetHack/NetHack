cmake_minimum_required(VERSION 3.15)
project(NetHack VERSION 3.6.6)

if(CMAKE_BUILD_TYPE)
  message("Build type: ${CMAKE_BUILD_TYPE}.")
endif()

message(STATUS "Building NetHack version: ${NetHack_VERSION}.")

set(HACKDIR
    "$ENV{HOME}/nethackdir"
    CACHE STRING "Configuration files for nethack")

message(STATUS "HACKDIR set to: ${HACKDIR}")

# Playground vars
set(VARDIR ${HACKDIR})
set(INSTDIR ${HACKDIR})

add_compile_definitions(
  GCC_WARN
  NOCLIPPING
  NOMAIL
  NOTPARMDECL
  AVOID_WIN_IOCTL
  SCORE_ON_BOTL
  # ANSI_DEFAULT NO_SIGNAL
  HACKDIR="${HACKDIR}"
  DEFAULT_WINDOW_SYS="tty"
  DLB)

set(NH_SRC ${NetHack_SOURCE_DIR}/src)
set(NH_INC ${NetHack_SOURCE_DIR}/include)
set(NH_DAT ${NetHack_SOURCE_DIR}/dat)
set(NH_UTIL ${NetHack_SOURCE_DIR}/util)
set(NH_DOC ${NetHack_SOURCE_DIR}/doc)

set(NH_SRC_GEN ${NetHack_BINARY_DIR}/src)
set(NH_INC_GEN ${NetHack_BINARY_DIR}/include)
set(NH_DAT_GEN ${NetHack_BINARY_DIR}/dat)
set(NH_UTIL_GEN ${NetHack_BINARY_DIR}/util)

add_subdirectory(util)
add_subdirectory(dat)

file(
  GLOB
  NETHACK_SRC
  "src/*.c"
  "sys/share/posixregex.c"
  "sys/share/ioctl.c"
  "sys/unix/unixunix.c"
  "sys/unix/unixmain.c"
  "sys/unix/unixres.c"
  "sys/share/unixtty.c"
  "win/tty/*.c")

# nethack executable.
add_executable(nethack ${NETHACK_SRC})
target_link_libraries(nethack ncurses)

add_dependencies(nethack util dat)

target_include_directories(
  nethack
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${NH_INC_GEN} /usr/local/include)
