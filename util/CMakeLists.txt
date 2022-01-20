set(MAKEDEFS_SRC makedefs.c ${NH_SRC}/monst.c ${NH_SRC}/objects.c
                 ${NH_SRC}/alloc.c ${NH_SRC}/date.c)

# TODO: Re-add onames.h once "prob error for class 4 (28%)" error is fixed.
set(MAKEDEFS_HEADERS ${NH_INC_GEN}/date.h # ${NH_INC_GEN}/onames.h
                     ${NH_INC_GEN}/pm.h)

set(DLB_SRC dlb_main.c ${NH_SRC}/dlb.c panic.c ${NH_SRC}/alloc.c)

file(MAKE_DIRECTORY ${NH_INC_GEN})

add_executable(makedefs ${MAKEDEFS_SRC})
target_include_directories(makedefs PRIVATE ${NH_INC} ${NH_INC_GEN}
                                            ${NH_UTIL_GEN} ${LUA_INCLUDE})

add_executable(dlb ${DLB_SRC})
target_include_directories(dlb PRIVATE ${NH_INC} ${NH_INC_GEN} ${NH_UTIL_GEN}
                                       ${LUA_INCLUDE})

add_executable(recover recover.c)
target_include_directories(recover PRIVATE ${NH_INC} ${NH_INC_GEN}
                                           ${NH_UTIL_GEN} ${LUA_INCLUDE})

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

add_custom_target(util DEPENDS ${MAKEDEFS_HEADERS} recover)

set(MAN_PAGES ${NH_DOC}/makedefs.6 ${NH_DOC}/dlb.6 ${NH_DOC}/recover.6
              ${NH_DOC}/nethack.6)

set(TXT_DOCS ${NH_DOC}/makedefs.txt ${NH_DOC}/dlb.txt ${NH_DOC}/recover.txt
             ${NH_DOC}/nethack.txt)

install(TARGETS makedefs recover DESTINATION ${INSTDIR})
install(FILES ${MAN_PAGES} DESTINATION ${INSTDIR}/man)
install(FILES ${TXT_DOCS} DESTINATION ${INSTDIR}/doc)
