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

add_executable(hikari_golden_h7_relevance_placement_diff_test
    h7_relevance_placement_diff_test.cpp
)

target_link_libraries(hikari_golden_h7_relevance_placement_diff_test
    PRIVATE
        hikari_validate
        hikari_erg
        hikari_rsb
        hikari_capture
        hikari_core
        hikari_warnings
)

add_test(NAME golden.h7_relevance_placement_diff COMMAND hikari_golden_h7_relevance_placement_diff_test)

add_executable(hikari_golden_speculative_override_test
    speculative_override_test.cpp
)

target_link_libraries(hikari_golden_speculative_override_test
    PRIVATE
        hikari_checkpoint
        hikari_warnings
)

add_test(NAME golden.speculative_override COMMAND hikari_golden_speculative_override_test)
