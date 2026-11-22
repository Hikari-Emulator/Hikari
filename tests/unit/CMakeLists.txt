add_executable(hikari_unit_capture_test
    capture_test.cpp
)

target_link_libraries(hikari_unit_capture_test
    PRIVATE
        hikari_capture
        hikari_core
        hikari_warnings
)

add_test(NAME unit.capture COMMAND hikari_unit_capture_test)

add_executable(hikari_unit_replay_alloc_test
    replay_alloc_test.cpp
)

target_link_libraries(hikari_unit_replay_alloc_test
    PRIVATE
        hikari_validate
        hikari_capture
        hikari_core
        hikari_warnings
)

add_test(NAME unit.replay_alloc COMMAND hikari_unit_replay_alloc_test)

add_executable(hikari_unit_rsb_erg_test
    rsb_erg_test.cpp
)

target_link_libraries(hikari_unit_rsb_erg_test
    PRIVATE
        hikari_validate
        hikari_erg
        hikari_rsb
        hikari_capture
        hikari_core
        hikari_warnings
)

add_test(NAME unit.rsb_erg COMMAND hikari_unit_rsb_erg_test)
