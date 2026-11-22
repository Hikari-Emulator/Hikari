add_library(hikari_rsb STATIC
    rsb.cpp
)

target_include_directories(hikari_rsb PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_rsb
    PUBLIC
        hikari_core
    PRIVATE
        hikari_warnings
)
