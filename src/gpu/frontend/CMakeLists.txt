add_library(hikari_gpu_frontend STATIC
    frontend.cpp
)

target_include_directories(hikari_gpu_frontend PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_gpu_frontend
    PUBLIC
        hikari_validate
        hikari_ir
        hikari_gpu_gen
        hikari_gpu_hct
        hikari_erg
        hikari_rsb
        hikari_capture
        hikari_core
    PRIVATE
        hikari_warnings
)
