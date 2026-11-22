add_executable(hikari_fault_injection_test
    fault_injection_test.cpp
)

target_link_libraries(hikari_fault_injection_test
    PRIVATE
        hikari_checkpoint
        hikari_warnings
)

add_test(NAME fault.self_recovery COMMAND hikari_fault_injection_test)
