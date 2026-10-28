# Unit tests executable
add_executable(test_netpower
    unit/test_main.cpp
)

target_link_libraries(test_netpower PRIVATE netpower_core)
target_include_directories(test_netpower PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
)

add_test(NAME unit_tests COMMAND test_netpower)

# Golden-file regression integration test
add_test(NAME regression_test
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/run_regression.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

