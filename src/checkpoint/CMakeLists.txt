add_library(hikari_checkpoint STATIC
    checkpoint.cpp
)

target_include_directories(hikari_checkpoint PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_checkpoint
    PUBLIC
        hikari_rsb
        hikari_core
    PRIVATE
        hikari_warnings
)
