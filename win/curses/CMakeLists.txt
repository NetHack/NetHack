find_package(Curses)
if(Curses_FOUND)
  include_directories(${CURSES_INCLUDE_DIRS})
  target_link_libraries(NetHack ${CURSES_LIBRARIES})
  target_sources(NetHack PUBLIC cursmain.c curswins.c cursmisc.c cursdial.c 
  cursstat.c cursinit.c cursmesg.c cursinvt.c)
endif()
