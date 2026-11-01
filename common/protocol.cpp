#include "protocol.h"
#include <nlohmann/json.hpp>

// ordered_json keeps keys in insertion order, so "type" is always first on the wire
// and lines stay easy to read by eye. Plain nlohmann::json would sort keys alphabetically.
using json = nlohmann::ordered_json;

namespace {
    bool has_string(const json& j, const char* key) {
        auto it = j.find(key);
        return it != j.end() && it->is_string();
    }

    bool has_int(const json& j, const char* key) {
        auto it = j.find(key);
        return it != j.end() && it->is_number_integer();
    }

    bool has_int_array(const json& j, const char* key) {
        auto it = j.find(key);
        if (it == j.end() || !it->is_array()) return false;
        for (const auto& element : *it) {
            if (!element.is_number_integer()) return false;
        }
        return true;
    }

    // Parses a line into a JSON object that has a string "type" field.
    // Uses the non-throwing parse, so malformed input gives nullopt, not an exception.
    std::optional<json> parse_object(const std::string& line) {
        json j = json::parse(line, nullptr, /*allow_exceptions=*/false);
        if (j.is_discarded() || !j.is_object() || !has_string(j, "type")) {
            return std::nullopt;
        }
        return j;
    }

    // error_handler_t::replace swaps invalid UTF-8 for U+FFFD instead of throwing,
    // so serializing can never take down a session thread.
    std::string finish(const json& j) {
        return j.dump(-1, ' ', false, json::error_handler_t::replace) + '\n';
    }
}

std::optional<ClientMessage> parse_client_message(const std::string& line) {
    auto j = parse_object(line);
    if (!j) return std::nullopt;
    const auto type = j->at("type").get<std::string>();

    if (type == "join" && has_string(*j, "name")) {
        return JoinRequest{j->at("name").get<std::string>()};
    }
    if (type == "roll" && has_string(*j, "expr")) {
        return RollRequest{j->at("expr").get<std::string>()};
    }
    if (type == "chat" && has_string(*j, "text")) {
        return ChatRequest{j->at("text").get<std::string>()};
    }
    return std::nullopt;
}

std::optional<ServerMessage> parse_server_message(const std::string& line) {
    auto j = parse_object(line);
    if (!j) return std::nullopt;
    const auto type = j->at("type").get<std::string>();

    if (type == "player_joined" && has_string(*j, "name")) {
        return PlayerJoined{j->at("name").get<std::string>()};
    }
    if (type == "roll_result" && has_string(*j, "player") && has_string(*j, "expr")
        && has_int_array(*j, "dice") && has_int(*j, "modifier") && has_int(*j, "total")) {
        return RollResultMessage{
            j->at("player").get<std::string>(),
            j->at("expr").get<std::string>(),
            j->at("dice").get<std::vector<int>>(),
            j->at("modifier").get<int>(),
            j->at("total").get<int>(),
        };
    }
    if (type == "chat" && has_string(*j, "player") && has_string(*j, "text")) {
        return ChatMessage{j->at("player").get<std::string>(), j->at("text").get<std::string>()};
    }
    if (type == "error" && has_string(*j, "message")) {
        return ErrorMessage{j->at("message").get<std::string>()};
    }
    return std::nullopt;
}

std::string to_line(const JoinRequest& msg) {
    return finish({{"type", "join"}, {"name", msg.name}});
}

std::string to_line(const RollRequest& msg) {
    return finish({{"type", "roll"}, {"expr", msg.expr}});
}

std::string to_line(const ChatRequest& msg) {
    return finish({{"type", "chat"}, {"text", msg.text}});
}

std::string to_line(const PlayerJoined& msg) {
    return finish({{"type", "player_joined"}, {"name", msg.name}});
}

std::string to_line(const RollResultMessage& msg) {
    return finish({
        {"type", "roll_result"},
        {"player", msg.player},
        {"expr", msg.expr},
        {"dice", msg.dice},
        {"modifier", msg.modifier},
        {"total", msg.total},
    });
}

std::string to_line(const ChatMessage& msg) {
    return finish({{"type", "chat"}, {"player", msg.player}, {"text", msg.text}});
}

std::string to_line(const ErrorMessage& msg) {
    return finish({{"type", "error"}, {"message", msg.message}});
}
