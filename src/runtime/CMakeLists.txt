add_library(hikari_runtime STATIC
    frame_loop.cpp
)

target_include_directories(hikari_runtime PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_runtime
    PUBLIC
        hikari_checkpoint
        hikari_exec
        hikari_gpu_frontend
        hikari_services
        hikari_cpu
        hikari_loader
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

add_executable(hikari_run
    hikari_run_main.cpp
)

target_link_libraries(hikari_run
    PRIVATE
        hikari_runtime
        hikari_warnings
)
