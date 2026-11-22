add_library(hikari_erg STATIC
    erg.cpp
    hct_stub.cpp
)

target_include_directories(hikari_erg PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_erg
    PUBLIC
        hikari_rsb
        hikari_capture
        hikari_core
    PRIVATE
        hikari_warnings
)
