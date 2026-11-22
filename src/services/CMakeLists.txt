add_library(hikari_services STATIC
    service_host.cpp
)

target_include_directories(hikari_services PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hikari_services
    PUBLIC
        hikari_core
    PRIVATE
        hikari_warnings
)
