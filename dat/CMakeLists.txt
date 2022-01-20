set(DATHELP
    help
    hh
    cmdhelp
    keyhelp
    history
    opthelp
    wizhelp)

set(SPECIAL_LEVELS
    bigroom.des
    castle.des
    endgame.des
    gehennom.des
    knox.des
    medusa.des
    mines.des
    oracle.des
    sokoban.des
    tower.des
    yendor.des)

set(QUEST_LEVELS
    Arch.des
    Barb.des
    Caveman.des
    Healer.des
    Knight.des
    Monk.des
    Priest.des
    Ranger.des
    Rogue.des
    Samurai.des
    Tourist.des
    Valkyrie.des
    Wizard.des)

set(DATDLB
    ${DATHELP}
    dungeon
    tribute
    bogusmon
    data
    engrave
    epitaph
    oracles
    options
    quest.dat
    rumors)

file(MAKE_DIRECTORY ${NH_DAT_GEN})

# Inputs to makedef and lev_comp (and transitive inputs to dgn_comp and dlb).
set(ALL_DAT_NOTGEN
    ${DATHELP}
    ${SPECIAL_LEVELS}
    ${QUEST_LEVELS}
    bogusmon.txt
    data.base
    dungeon.def
    engrave.txt
    epitaph.txt
    oracles.txt
    quest.txt
    rumors.fal
    rumors.tru
    tribute)

# makedef and friends like having their inputs in the same directory.
foreach(filename ${ALL_DAT_NOTGEN})
  configure_file(${NH_DAT}/${filename} ${filename} COPYONLY)
endforeach(filename)

# Dat files that are not inputs to any tools.
set(ALL_DAT_NOTGEN ${ALL_DAT_NOTGEN} license symbols)

# All dat/*.lev files.
file(GLOB LEVS_GEN RELATIVE ${NH_DAT} *.lev)

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

# Hack: Depend on bogusmon to avoid race wrt. makedef's grep.tmp file. Best to
# be defensive about the paths (matters if NetHack's make install was run).
add_custom_command(
  DEPENDS $<TARGET_FILE:makedefs> $<TARGET_FILE:dgn_comp>
          ${NH_DAT_GEN}/dungeon.def ${NH_DAT_GEN}/bogusmon
  OUTPUT dungeon dungeon.pdf
  COMMAND $<TARGET_FILE:makedefs> ARGS -e
  COMMAND $<TARGET_FILE:dgn_comp> ARGS dungeon.pdf)

add_custom_command(
  DEPENDS lev_comp
  OUTPUT ${LEVS_GEN}
  COMMAND $<TARGET_FILE:lev_comp> ${SPECIAL_LEVELS}
  COMMAND $<TARGET_FILE:lev_comp> ${QUEST_LEVELS})

add_custom_command(
  OUTPUT perm record logfile xlogfile sysconf
  COMMAND ${CMAKE_COMMAND} -E touch perm record logfile xlogfile
  COMMAND ${CMAKE_COMMAND} -E echo "WIZARDS=*" > sysconf)

# Defensive dependencies for when we also build NetHack with make/make install.
list(TRANSFORM DATDLB PREPEND ${NH_DAT_GEN}/ OUTPUT_VARIABLE
                                              NH_DAT_GEN_DATDLB)
list(TRANSFORM LEVS_GEN PREPEND ${NH_DAT_GEN}/ OUTPUT_VARIABLE
                                                NH_DAT_GEN_LEVS_GEN)

add_custom_command(
  DEPENDS dlb ${NH_DAT_GEN_DATDLB} ${NH_DAT_GEN_LEVS_GEN}
  OUTPUT nhdat
  COMMAND LC_ALL=C $<TARGET_FILE:dlb> ARGS cf nhdat ${DATDLB} ${LEVS_GEN})

set(ALL_DAT_GEN nhdat perm record logfile xlogfile sysconf)
list(TRANSFORM ALL_DAT_GEN PREPEND ${NH_DAT_GEN}/)

add_custom_target(dat ALL DEPENDS ${ALL_DAT_GEN} ${ALL_DAT_NOTGEN})

install(FILES ${ALL_DAT_GEN} DESTINATION ${INSTDIR})
install(FILES ${ALL_DAT_NOTGEN} DESTINATION ${INSTDIR}/dat)
install(DIRECTORY DESTINATION ${INSTDIR}/save)
