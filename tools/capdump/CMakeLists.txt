add_executable(hikari_capdump
    main.cpp
)

target_link_libraries(hikari_capdump
    PRIVATE
        hikari_capture
        hikari_core
        hikari_warnings
)
