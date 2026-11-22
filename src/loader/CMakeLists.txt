add_library(hikari_loader STATIC
    loader.cpp
)

target_include_directories(hikari_loader PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_loader
    PUBLIC
        hikari_core
    PRIVATE
        hikari_warnings
)
