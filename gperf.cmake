find_program(GPERF_EXECUTABLE NAMES gperf)

add_custom_command(OUTPUT "${CMAKE_SOURCE_DIR}/include/keywords.h"
    COMMAND ${GPERF_EXECUTABLE} --output-file=${CMAKE_SOURCE_DIR}/include/keywords.h ${CMAKE_SOURCE_DIR}/include/keywords.gperf)

add_custom_target(generate_keyword_hashmap ALL
    DEPENDS "${CMAKE_SOURCE_DIR}/include/keywords.h")

install(FILES ${CMAKE_SOURCE_DIR}/include/keywords.h DESTINATION include)