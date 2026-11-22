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
