add_executable(hikari_golden_determinism_test
    determinism_test.cpp
)

target_link_libraries(hikari_golden_determinism_test
    PRIVATE
        hikari_validate
        hikari_capture
        hikari_core
        hikari_warnings
)

add_test(NAME golden.determinism COMMAND hikari_golden_determinism_test)

add_executable(hikari_golden_fixture_relations_test
    fixture_relations_test.cpp
)

target_link_libraries(hikari_golden_fixture_relations_test
    PRIVATE
        hikari_validate
        hikari_capture
        hikari_core
        hikari_warnings
)

add_test(NAME golden.fixture_relations COMMAND hikari_golden_fixture_relations_test)

add_executable(hikari_golden_rsb_erg_diff_test
    rsb_erg_diff_test.cpp
)

target_link_libraries(hikari_golden_rsb_erg_diff_test
    PRIVATE
        hikari_validate
        hikari_erg
        hikari_rsb
        hikari_capture
        hikari_core
        hikari_warnings
)

add_test(NAME golden.rsb_erg_diff COMMAND hikari_golden_rsb_erg_diff_test)
