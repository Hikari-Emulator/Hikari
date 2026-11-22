add_library(hikari_gpu_gen STATIC
    generations.cpp
)

target_include_directories(hikari_gpu_gen PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_gpu_gen
    PUBLIC
        hikari_capture
        hikari_core
    PRIVATE
        hikari_warnings
)
