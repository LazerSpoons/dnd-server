add_executable(tests test_dice.cpp test_protocol.cpp)
target_link_libraries(tests PRIVATE common Catch2::Catch2WithMain)
add_test(NAME unit_tests COMMAND tests)