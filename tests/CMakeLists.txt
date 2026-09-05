add_executable(tests test_dice.cpp)
target_link_libraries(tests PRIVATE common Catch2::Catch2WithMain)
add_test(NAME dice COMMAND tests)