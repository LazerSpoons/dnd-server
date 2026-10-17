#include <catch2/catch_test_macros.hpp>
#include "dice.h"

TEST_CASE("d1 rolls one") {
    auto r = roll("1d1");
    REQUIRE(r);
    REQUIRE(r->total == 1);
}
TEST_CASE("d6 rolls between 1 and 6"){
    for(int i = 0; i < 100; ++i){
        auto r = roll("1d6");
        REQUIRE(r);
        REQUIRE(r->total >= 1);
        REQUIRE(r->total <= 6);
    }
}
TEST_CASE("Can't roll 0 dice") {
    auto r = roll("0d6");
    REQUIRE(!r);
}
TEST_CASE("Can't roll negative dice") {
    auto r = roll("-1d6");
    REQUIRE(!r);
}
TEST_CASE("Can't roll empty string") {
    auto r = roll("");
    REQUIRE(!r);
}
TEST_CASE("Can't roll 2d") {
    auto r = roll("2d");
    REQUIRE(!r);
}
TEST_CASE("Cant roll a plain d") {
    auto r = roll("d");
    REQUIRE(!r);
}
TEST_CASE("Can't roll 2x6"){
    auto r = roll("2x6");
    REQUIRE(!r);
}
TEST_CASE("Can't roll 2d6d6"){
    auto r = roll("2d6d6");
    REQUIRE(!r);
}
TEST_CASE("Can't roll 2d6+"){
    auto r = roll("2d6+");
    REQUIRE(!r);
}
TEST_CASE("999999d6 is out of range") {
    auto r = roll("999999d6");
    REQUIRE(!r);
}
TEST_CASE("1d1-5 is -4") {
    auto r = roll("1d1-5");
    
    REQUIRE(r);
    REQUIRE(r->modifier == -5);
    REQUIRE(r->total == -4);
}
TEST_CASE("d20 works"){
    auto r = roll("d20");
    REQUIRE(r);
    REQUIRE(r->dice.size() == 1);
    REQUIRE(r->total >= 1);
    REQUIRE(r->total <= 20);
}
TEST_CASE("multidice rolls work"){
    auto r = roll("4d1");

    REQUIRE(r);
    REQUIRE(r->dice.size() == 4);
    for(int d : r->dice){
        REQUIRE(d == 1);
    }
    REQUIRE(r->total >= 4);
    REQUIRE(r->total <= 4);

}
TEST_CASE("1d999999 is out of range") {
    auto r = roll("1d999999");
    REQUIRE(!r);
}