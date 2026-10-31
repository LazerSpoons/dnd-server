#pragma once
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Wire protocol: one JSON object per line, terminated with '\n'.
// Every message has a string "type" field that says which struct it maps to.

// ---- Client -> server -------------------------------------------------------

// {"type":"join","name":"Somil"}
struct JoinRequest {
    std::string name;
};

// {"type":"roll","expr":"2d6+3"}
struct RollRequest {
    std::string expr;
};

// {"type":"chat","text":"I check for traps"}
struct ChatRequest {
    std::string text;
};

using ClientMessage = std::variant<JoinRequest, RollRequest, ChatRequest>;

// ---- Server -> client -------------------------------------------------------

// {"type":"player_joined","name":"Somil"}
struct PlayerJoined {
    std::string name;
};

// {"type":"roll_result","player":"Somil","expr":"2d6+3","dice":[4,6],"total":13,"modifier":3}
struct RollResultMessage {
    std::string player;
    std::string expr;
    std::vector<int> dice;
    int modifier = 0;
    int total = 0;
};

// {"type":"chat","player":"Somil","text":"I check for traps"}
struct ChatMessage {
    std::string player;
    std::string text;
};

// {"type":"error","message":"unknown dice expression"}
struct ErrorMessage {
    std::string message;
};

using ServerMessage = std::variant<PlayerJoined, RollResultMessage, ChatMessage, ErrorMessage>;

// ---- Parsing ----------------------------------------------------------------

// Parse one line (with or without its trailing newline). Returns std::nullopt for
// invalid JSON, a non-object, a missing or unknown "type", or a missing or
// wrong-typed field. Extra fields are ignored. Never throws.
std::optional<ClientMessage> parse_client_message(const std::string& line);
std::optional<ServerMessage> parse_server_message(const std::string& line);

// ---- Serializing ------------------------------------------------------------

// Each returns exactly one line: compact JSON followed by a single '\n', ready to
// write to a socket. Newlines inside strings are escaped, so a message can never
// span two lines. Never throws (invalid UTF-8 is replaced, not rejected).
std::string to_line(const JoinRequest& msg);
std::string to_line(const RollRequest& msg);
std::string to_line(const ChatRequest& msg);
std::string to_line(const PlayerJoined& msg);
std::string to_line(const RollResultMessage& msg);
std::string to_line(const ChatMessage& msg);
std::string to_line(const ErrorMessage& msg);
