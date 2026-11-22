add_executable(hikari_integration_runtime_scaffold_test
    runtime_scaffold_test.cpp
)

target_link_libraries(hikari_integration_runtime_scaffold_test
    PRIVATE
        hikari_runtime
        hikari_warnings
)

add_test(NAME integration.runtime_scaffold COMMAND hikari_integration_runtime_scaffold_test)
