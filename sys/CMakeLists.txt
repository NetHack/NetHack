target_sources(NetHack PUBLIC share/ioctl.c share/unixtty.c 
  unix/unixmain.c unix/unixunix.c unix/unixres.c)

add_subdirectory(share)
