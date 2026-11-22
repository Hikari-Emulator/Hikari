add_library(hikari_validate STATIC
    reference_replay.cpp
    rsb_erg_replay.cpp
)

target_include_directories(hikari_validate PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_validate
    PUBLIC
        hikari_erg
        hikari_rsb
        hikari_capture
        hikari_core
    PRIVATE
        hikari_warnings
)

add_executable(hikari_ref_replay
    ref_replay_main.cpp
)

target_link_libraries(hikari_ref_replay
    PRIVATE
        hikari_validate
        hikari_capture
        hikari_core
        hikari_warnings
)
