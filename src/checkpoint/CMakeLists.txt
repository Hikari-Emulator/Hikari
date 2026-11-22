add_library(hikari_checkpoint STATIC
    checkpoint.cpp
    speculative_override.cpp
)

target_include_directories(hikari_checkpoint PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_checkpoint
    PUBLIC
        hikari_validate
        hikari_erg
        hikari_rsb
        hikari_capture
        hikari_core
    PRIVATE
        hikari_warnings
)
