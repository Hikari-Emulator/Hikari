add_library(hikari_cpu STATIC
    cpu_backend.cpp
)

target_include_directories(hikari_cpu PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_cpu
    PUBLIC
        hikari_core
    PRIVATE
        hikari_warnings
)
