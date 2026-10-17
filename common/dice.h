#pragma once
#include <optional>
#include <string>
#include <vector>

struct RollResult {
    std::vector<int> dice;
    int modifier = 0;
    int total = 0;
};

// Returns std::nullopt if expr is malformed or out of range.
std::optional<RollResult> roll(const std::string& expr);