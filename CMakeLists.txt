cmake_minimum_required(VERSION 3.15)

project(NetHack VERSION 5.0.0)
set(CMAKE_C_STANDARD 99)


include("CMakeOptions.txt")

if(CMAKE_BUILD_TYPE)
  message("Build type: ${CMAKE_BUILD_TYPE}.")
endif()

message(STATUS "Building NetHack version: ${NetHack_VERSION}.")

include_directories(include)

add_executable(NetHack)

if(WANT_WIN_CURSES)
  add_compile_definitions(CURSES_GRAPHICS)
endif()

if(NOT LUA_SUPPORT EQUAL No)
  find_package(Lua 5.4)
  if(Lua_FOUND)
    include_directories(SYSTEM ${LUA_INCLUDE_DIR})
    add_compile_definitions(NETHACK_LUA_SUPPORT)
    target_link_libraries(NetHack ${LUA_LIBRARIES})
  elseif(LUA_SUPPORT EQUAL Yes)
    message(FATAL "LUA_SUPPORT was set to 'Yes' but Lua 5.4 was not found")
  endif()
endif()

include(CheckSymbolExists)

# TODO this works incorrectly for windows, probably...
check_symbol_exists(backtrace "execinfo.h" HAVE_BACKTRACE)

if(NOT HAVE_BACKTRACE)
  add_compile_definitions(NetHack PUBLIC NO_BACKTRACE)
endif()

add_compile_definitions(NetHack PUBLIC SYSCF
  HACKDIR=\"${HACKDIR}\")

add_subdirectory(win)
add_subdirectory(src)
add_subdirectory(sys)
add_subdirectory(util)
