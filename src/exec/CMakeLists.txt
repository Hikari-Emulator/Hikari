add_library(hikari_exec STATIC
    gfx/executor.cpp
)

target_include_directories(hikari_exec PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_exec
    PUBLIC
        hikari_ir
        hikari_rsb
        hikari_capture
        hikari_core
    PRIVATE
        hikari_warnings
)
