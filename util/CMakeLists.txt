add_executable(dlb 
  dlb_main.c 
  ${CMAKE_SOURCE_DIR}/src/dlb.c
  ${CMAKE_SOURCE_DIR}/src/alloc.c 
  panic.c
)
target_compile_definitions(dlb PUBLIC DLB)
target_link_libraries(dlb hacklib)
