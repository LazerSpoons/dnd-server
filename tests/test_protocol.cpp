#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include "protocol.h"

using nlohmann::json;

// ---- Parsing client messages ------------------------------------------------

TEST_CASE("Parses a join request") {
    auto msg = parse_client_message(R"({"type":"join","name":"Somil"})");
    REQUIRE(msg);
    REQUIRE(std::holds_alternative<JoinRequest>(*msg));
    REQUIRE(std::get<JoinRequest>(*msg).name == "Somil");
}

TEST_CASE("Parses a roll request") {
    auto msg = parse_client_message(R"({"type":"roll","expr":"2d6+3"})");
    REQUIRE(msg);
    REQUIRE(std::holds_alternative<RollRequest>(*msg));
    REQUIRE(std::get<RollRequest>(*msg).expr == "2d6+3");
}

TEST_CASE("Parses a chat request") {
    auto msg = parse_client_message(R"({"type":"chat","text":"I check for traps"})");
    REQUIRE(msg);
    REQUIRE(std::holds_alternative<ChatRequest>(*msg));
    REQUIRE(std::get<ChatRequest>(*msg).text == "I check for traps");
}

TEST_CASE("Parsing tolerates a trailing newline and carriage return") {
    REQUIRE(parse_client_message("{\"type\":\"roll\",\"expr\":\"d20\"}\n"));
    REQUIRE(parse_client_message("{\"type\":\"roll\",\"expr\":\"d20\"}\r\n"));
}

TEST_CASE("Extra fields are ignored") {
    auto msg = parse_client_message(R"({"type":"roll","expr":"d20","extra":true})");
    REQUIRE(msg);
    REQUIRE(std::get<RollRequest>(*msg).expr == "d20");
}

TEST_CASE("Rejects malformed client messages") {
    REQUIRE_FALSE(parse_client_message(""));
    REQUIRE_FALSE(parse_client_message("hello"));                          // not JSON
    REQUIRE_FALSE(parse_client_message(R"({"type":"roll","expr":"d20")"));  // truncated
    REQUIRE_FALSE(parse_client_message("42"));                             // not an object
    REQUIRE_FALSE(parse_client_message(R"(["join"])"));                    // not an object
    REQUIRE_FALSE(parse_client_message(R"({"name":"Somil"})"));            // no type
    REQUIRE_FALSE(parse_client_message(R"({"type":5,"name":"Somil"})"));   // type not a string
    REQUIRE_FALSE(parse_client_message(R"({"type":"dance"})"));            // unknown type
    REQUIRE_FALSE(parse_client_message(R"({"type":"join"})"));             // missing field
    REQUIRE_FALSE(parse_client_message(R"({"type":"join","name":5})"));    // wrong field type
    REQUIRE_FALSE(parse_client_message(R"({"type":"roll","expr":null})")); // wrong field type
}

TEST_CASE("Server-only types are not accepted from clients") {
    REQUIRE_FALSE(parse_client_message(R"({"type":"player_joined","name":"Somil"})"));
    REQUIRE_FALSE(parse_client_message(R"({"type":"error","message":"x"})"));
}

// ---- Serializing ------------------------------------------------------------

TEST_CASE("roll_result serializes with the documented field names") {
    RollResultMessage msg{"Somil", "2d6+3", {4, 6}, 3, 13};
    auto line = to_line(msg);
    auto j = json::parse(line);
    REQUIRE(j["type"] == "roll_result");
    REQUIRE(j["player"] == "Somil");
    REQUIRE(j["expr"] == "2d6+3");
    REQUIRE(j["dice"] == json::array({4, 6}));
    REQUIRE(j["modifier"] == 3);
    REQUIRE(j["total"] == 13);
}

TEST_CASE("\"type\" is the first key on the wire") {
    // Readability by eye in nc/Telnet: the message type should lead every line.
    REQUIRE(to_line(RollResultMessage{"Somil", "d20", {7}, 0, 7}).starts_with(R"({"type":"roll_result",)"));
    REQUIRE(to_line(ErrorMessage{"oops"}).starts_with(R"({"type":"error",)"));
    REQUIRE(to_line(JoinRequest{"Somil"}).starts_with(R"({"type":"join",)"));
}

TEST_CASE("Every serialized message is exactly one line") {
    // A newline inside a string must be escaped, or it would split the message in two.
    auto line = to_line(ChatMessage{"Somil", "line one\nline two"});
    REQUIRE(line.back() == '\n');
    REQUIRE(std::count(line.begin(), line.end(), '\n') == 1);

    auto parsed = parse_server_message(line);
    REQUIRE(parsed);
    REQUIRE(std::get<ChatMessage>(*parsed).text == "line one\nline two");
}

TEST_CASE("Serializing invalid UTF-8 does not throw") {
    std::string bad = "caf\xC3";  // truncated multi-byte sequence
    REQUIRE_NOTHROW(to_line(ChatMessage{"Somil", bad}));
    REQUIRE_NOTHROW(to_line(ErrorMessage{bad}));
}

// ---- Round trips ------------------------------------------------------------

TEST_CASE("Client messages survive a round trip") {
    auto join = parse_client_message(to_line(JoinRequest{"Somil"}));
    REQUIRE(join);
    REQUIRE(std::get<JoinRequest>(*join).name == "Somil");

    auto roll = parse_client_message(to_line(RollRequest{"4d8-2"}));
    REQUIRE(roll);
    REQUIRE(std::get<RollRequest>(*roll).expr == "4d8-2");

    auto chat = parse_client_message(to_line(ChatRequest{"hi \"there\""}));
    REQUIRE(chat);
    REQUIRE(std::get<ChatRequest>(*chat).text == "hi \"there\"");
}

TEST_CASE("Server messages survive a round trip") {
    auto joined = parse_server_message(to_line(PlayerJoined{"Somil"}));
    REQUIRE(joined);
    REQUIRE(std::get<PlayerJoined>(*joined).name == "Somil");

    auto result = parse_server_message(to_line(RollResultMessage{"Somil", "4d8-2", {3, 8, 1, 5}, -2, 15}));
    REQUIRE(result);
    const auto& r = std::get<RollResultMessage>(*result);
    REQUIRE(r.player == "Somil");
    REQUIRE(r.expr == "4d8-2");
    REQUIRE(r.dice == std::vector<int>{3, 8, 1, 5});
    REQUIRE(r.modifier == -2);
    REQUIRE(r.total == 15);

    auto error = parse_server_message(to_line(ErrorMessage{"bad dice"}));
    REQUIRE(error);
    REQUIRE(std::get<ErrorMessage>(*error).message == "bad dice");
}

TEST_CASE("Rejects malformed server messages") {
    REQUIRE_FALSE(parse_server_message(R"({"type":"roll_result","player":"Somil"})"));  // missing fields
    REQUIRE_FALSE(parse_server_message(
        R"({"type":"roll_result","player":"Somil","expr":"d6","dice":["4"],"modifier":0,"total":4})"));  // dice not ints
    REQUIRE_FALSE(parse_server_message(R"({"type":"join","name":"Somil"})"));  // client-only type
}
