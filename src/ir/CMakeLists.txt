add_library(hikari_ir STATIC
    ir_packet.cpp
)

target_include_directories(hikari_ir PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_ir
    PUBLIC
        hikari_rsb
        hikari_capture
        hikari_core
    PRIVATE
        hikari_warnings
)
