add_library(hikari_capture STATIC
    fixture.cpp
    reader.cpp
    writer.cpp
)

target_include_directories(hikari_capture PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_capture
    PUBLIC
        hikari_core
    PRIVATE
        hikari_warnings
)
