add_library(hikari_gpu_hct STATIC
    hct.cpp
)

target_include_directories(hikari_gpu_hct PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_gpu_hct
    PUBLIC
        hikari_erg
        hikari_rsb
        hikari_capture
        hikari_core
    PRIVATE
        hikari_warnings
)
