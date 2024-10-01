set(VARDATND)

set(VARDATD
    bogusmon
    data
    engrave
    epitaph
    oracles
    options
    quest.lua
    rumors)

set(VARDAT ${VARDATD} ${VARDATND})

set(DATHELP
    help
    hh
    cmdhelp
    keyhelp
    history
    opthelp
    optmenu
    wizhelp)

file(
  GLOB SPEC_LEVS
  RELATIVE ${NH_DAT}
  asmodeus.lua
  baalz.lua
  bigrm-*.lua
  castle.lua
  fakewiz?.lua
  juiblex.lua
  knox.lua
  medusa-?.lua
  minend-?.lua
  minefill.lua
  minetn-?.lua
  oracle.lua
  orcus.lua
  sanctum.lua
  soko?-?.lua
  tower?.lua
  valley.lua
  wizard?.lua
  nhcore.lua
  nhlib.lua
  themerms.lua
  astral.lua
  air.lua
  earth.lua
  fire.lua
  water.lua)

file(
  GLOB QUEST_LEVS
  RELATIVE ${NH_DAT}
  ???-goal.lua ???-fil?.lua ???-loca.lua ???-strt.lua)

set(DATNODLB ${VARDATND} license symbols)

set(DATDLB ${DATHELP} dungeon.lua tribute ${SPEC_LEVS} ${QUEST_LEVS} ${VARDATD})

set(DAT ${DATNODLB} ${DATDLB})

file(MAKE_DIRECTORY ${NH_DAT_GEN})

# Inputs to makedef.
set(ALL_DAT_NOTGEN
    ${DATHELP}
    ${SPEC_LEVS}
    ${QUEST_LEVS}
    bogusmon.txt
    data.base
    dungeon.lua
    engrave.txt
    epitaph.txt
    oracles.txt
    quest.lua
    rumors.fal
    rumors.tru
    tribute)

# makedef and friends like having their inputs in the same directory.
foreach(filename ${ALL_DAT_NOTGEN})
  configure_file(${NH_DAT}/${filename} ${filename} COPYONLY)
endforeach(filename)

# Dat files that are not inputs to any tools.
set(ALL_DAT_NOTGEN ${ALL_DAT_NOTGEN} ${DATNODLB})

add_custom_command(
  DEPENDS $<TARGET_FILE:makedefs> bogusmon.txt engrave.txt epitaph.txt
  OUTPUT bogusmon epitaph engrave
  COMMAND $<TARGET_FILE:makedefs> ARGS -s)

add_custom_command(
  DEPENDS $<TARGET_FILE:makedefs> data.base
  OUTPUT data
  COMMAND $<TARGET_FILE:makedefs> ARGS -d)

add_custom_command(
  DEPENDS $<TARGET_FILE:makedefs> rumors.tru rumors.fal
  OUTPUT rumors
  COMMAND $<TARGET_FILE:makedefs> ARGS -r)

add_custom_command(
  DEPENDS $<TARGET_FILE:makedefs> quest.txt
  OUTPUT quest.dat
  COMMAND $<TARGET_FILE:makedefs> ARGS -q)

add_custom_command(
  DEPENDS $<TARGET_FILE:makedefs> oracles.txt
  OUTPUT oracles
  COMMAND $<TARGET_FILE:makedefs> ARGS -h)

# Hack: Depend on date.h to avoid race.
add_custom_command(
  DEPENDS $<TARGET_FILE:makedefs> ${NH_INC_GEN}/date.h
  OUTPUT options
  COMMAND $<TARGET_FILE:makedefs> ARGS -v)

add_custom_command(
  OUTPUT perm record logfile xlogfile sysconf
  COMMAND ${CMAKE_COMMAND} -E touch perm record logfile xlogfile
  COMMAND ${CMAKE_COMMAND} -E echo "WIZARDS=*" > sysconf)

# Defensive dependencies for when we also build NetHack with make/make install.
list(TRANSFORM DATDLB PREPEND ${NH_DAT_GEN}/ OUTPUT_VARIABLE NH_DAT_GEN_DATDLB)

add_custom_command(
  DEPENDS dlb ${NH_DAT_GEN_DATDLB}
  OUTPUT nhdat
  COMMAND LC_ALL=C $<TARGET_FILE:dlb> ARGS cf nhdat ${DATDLB})

set(ALL_DAT_GEN nhdat perm record logfile xlogfile sysconf)
list(TRANSFORM ALL_DAT_GEN PREPEND ${NH_DAT_GEN}/)

add_custom_target(dat ALL DEPENDS ${ALL_DAT_GEN} ${ALL_DAT_NOTGEN} ${VARDAT})

install(FILES ${ALL_DAT_GEN} DESTINATION ${INSTDIR})
install(FILES ${ALL_DAT_NOTGEN} DESTINATION ${INSTDIR}/dat)
install(DIRECTORY DESTINATION ${INSTDIR}/save)
