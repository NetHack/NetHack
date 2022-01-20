find_program(FLEX_EXECUTABLE NAMES flex flex.exe)
find_program(BISON_EXECUTABLE NAMES bison bison.exe)

set(MAKEDEFS_SRC makedefs.c ${NH_SRC}/monst.c ${NH_SRC}/objects.c)

set(DGN_COMP_SRC dgn_main.c panic.c ${NH_SRC}/alloc.c
                 ${NH_UTIL_GEN}/dgn_parser.c ${NH_UTIL_GEN}/dgn_scanner.c)

set(LEV_COMP_SRC
    lev_main.c
    panic.c
    ${NH_SRC}/decl.c
    ${NH_SRC}/monst.c
    ${NH_SRC}/objects.c
    ${NH_SRC}/alloc.c
    ${NH_SRC}/drawing.c
    ${NH_UTIL_GEN}/lev_parser.c
    ${NH_UTIL_GEN}/lev_scanner.c)

set(MAKEDEFS_HEADERS ${NH_INC_GEN}/date.h ${NH_INC_GEN}/onames.h
                     ${NH_INC_GEN}/pm.h)

set(DLB_SRC dlb_main.c ${NH_SRC}/dlb.c panic.c ${NH_SRC}/alloc.c)

file(MAKE_DIRECTORY ${NH_INC_GEN})

add_executable(makedefs ${MAKEDEFS_SRC})
target_include_directories(makedefs PRIVATE ${NH_INC} ${NH_INC_GEN}
                                            ${NH_UTIL_GEN})
add_executable(dgn_comp ${DGN_COMP_SRC})
target_include_directories(dgn_comp PRIVATE ${NH_INC} ${NH_INC_GEN}
                                            ${NH_UTIL_GEN})
add_executable(lev_comp ${LEV_COMP_SRC})
target_include_directories(lev_comp PRIVATE ${NH_INC} ${NH_INC_GEN}
                                            ${NH_UTIL_GEN})
add_executable(dlb ${DLB_SRC})
target_include_directories(dlb PRIVATE ${NH_INC} ${NH_INC_GEN}
                                       ${NH_UTIL_GEN})
add_executable(recover recover.c)
target_include_directories(recover PRIVATE ${NH_INC} ${NH_INC_GEN}
                                           ${NH_UTIL_GEN})

add_custom_command(
  DEPENDS makedefs
  OUTPUT ${NH_INC_GEN}/date.h
  COMMAND $<TARGET_FILE:makedefs> ARGS -v)

add_custom_command(
  DEPENDS makedefs
  OUTPUT ${NH_INC_GEN}/onames.h
  COMMAND $<TARGET_FILE:makedefs> ARGS -o)

add_custom_command(
  DEPENDS makedefs
  OUTPUT ${NH_INC_GEN}/pm.h
  COMMAND $<TARGET_FILE:makedefs> ARGS -p)

add_custom_command(
  OUTPUT ${NH_UTIL_GEN}/dgn_parser.c ${NH_UTIL_GEN}/dgn_comp.h
  COMMAND bison -y --defines=${NH_UTIL_GEN}/dgn_comp.h -o
          ${NH_UTIL_GEN}/dgn_parser.c ${NH_UTIL}/dgn_comp.y
  DEPENDS ${NH_UTIL}/dgn_comp.y)

add_custom_command(
  OUTPUT ${NH_UTIL_GEN}/dgn_scanner.c
  COMMAND ${FLEX_EXECUTABLE} ARGS -o${NH_UTIL_GEN}/dgn_scanner.c
          ${NH_UTIL}/dgn_comp.l
  DEPENDS ${NH_UTIL}/dgn_comp.l)

add_custom_command(
  OUTPUT ${NH_UTIL_GEN}/lev_parser.c ${NH_UTIL_GEN}/lev_comp.h
  COMMAND ${BISON_EXECUTABLE} ARGS -y --defines=${NH_UTIL_GEN}/lev_comp.h -o
          ${NH_UTIL_GEN}/lev_parser.c ${NH_UTIL}/lev_comp.y
  DEPENDS ${NH_UTIL}/lev_comp.y)

add_custom_command(
  OUTPUT ${NH_UTIL_GEN}/lev_scanner.c
  COMMAND ${FLEX_EXECUTABLE} ARGS -o${NH_UTIL_GEN}/lev_scanner.c
          ${NH_UTIL}/lev_comp.l
  DEPENDS ${NH_UTIL}/lev_comp.l)

add_custom_target(util DEPENDS ${MAKEDEFS_HEADERS} recover)

# NOTE: util is dependent on these two
add_dependencies(lev_comp util)
add_dependencies(dgn_comp util)

set(MAN_PAGES ${NH_DOC}/makedefs.6 ${NH_DOC}/dgn_comp.6 ${NH_DOC}/lev_comp.6
              ${NH_DOC}/dlb.6 ${NH_DOC}/recover.6 ${NH_DOC}/nethack.6)

set(TXT_DOCS
    ${NH_DOC}/makedefs.txt ${NH_DOC}/dgn_comp.txt ${NH_DOC}/lev_comp.txt
    ${NH_DOC}/dlb.txt ${NH_DOC}/recover.txt ${NH_DOC}/nethack.txt)

install(TARGETS makedefs dgn_comp lev_comp dlb recover DESTINATION ${INSTDIR})
install(FILES ${MAN_PAGES} DESTINATION ${INSTDIR}/man)
install(FILES ${TXT_DOCS} DESTINATION ${INSTDIR}/doc)
