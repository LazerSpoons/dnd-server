#include <catch2/catch_test_macros.hpp>
#include "dice.h"

TEST_CASE("d1 rolls one") {
    auto r = roll("1d1");
    REQUIRE(r.total == 1);
}