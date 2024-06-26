find_program(GPERF_EXECUTABLE NAMES gperf)

add_custom_command(OUTPUT "${CMAKE_SOURCE_DIR}/src/keywords.cc"
    COMMAND ${GPERF_EXECUTABLE} --output-file=${CMAKE_SOURCE_DIR}/src/keywords.cc ${CMAKE_SOURCE_DIR}/src/keywords.gperf)

add_custom_target(generate_keyword_hashmap ALL
    DEPENDS "${CMAKE_SOURCE_DIR}/src/keywords.cc")

install(FILES ${CMAKE_SOURCE_DIR}/src/keywords.cc DESTINATION src)