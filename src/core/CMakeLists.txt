add_library(hikari_core STATIC
    assert.cpp
)

target_include_directories(hikari_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_core PRIVATE
    hikari_warnings
)
