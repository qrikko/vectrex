file(GLOB_RECURSE COSMIC_CHASM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_executable(cosmic-chasm ${COSMIC_CHASM_SOURCES})

target_include_directories(cosmic-chasm PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(cosmic-chasm PRIVATE
    GAME_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/"
)

target_link_libraries(cosmic-chasm PRIVATE raylib)
